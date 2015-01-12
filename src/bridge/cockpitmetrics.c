/*
 * This file is part of Cockpit.
 *
 * Copyright (C) 2014 Red Hat, Inc.
 *
 * Cockpit is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * Cockpit is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Cockpit; If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "cockpitmetrics.h"
#include "cockpitpcpmetrics.h"

#include "common/cockpitjson.h"

struct _CockpitMetricsPrivate {
  guint timeout;
  gint64 next;
  gint64 interval;
};

G_DEFINE_ABSTRACT_TYPE (CockpitMetrics, cockpit_metrics, COCKPIT_TYPE_CHANNEL);

static void
cockpit_metrics_init (CockpitMetrics *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, COCKPIT_TYPE_METRICS,
                                            CockpitMetricsPrivate);
}

static void
cockpit_metrics_recv (CockpitChannel *channel,
                      GBytes *message)
{
  g_warning ("received unexpected metrics1 payload");
  cockpit_channel_close (channel, "protocol-error");
}

static void
cockpit_metrics_close (CockpitChannel *channel,
                       const gchar *problem)
{
  CockpitMetrics *self = COCKPIT_METRICS (channel);

  if (self->priv->timeout)
    {
      g_source_remove (self->priv->timeout);
      self->priv->timeout = 0;
    }

  COCKPIT_CHANNEL_CLASS (cockpit_metrics_parent_class)->close (channel, problem);
}

static void
cockpit_metrics_dispose (GObject *object)
{
  CockpitMetrics *self = COCKPIT_METRICS (object);

  if (self->priv->timeout)
    {
      g_source_remove (self->priv->timeout);
      self->priv->timeout = 0;
    }

  G_OBJECT_CLASS (cockpit_metrics_parent_class)->dispose (object);
}

static void
cockpit_metrics_class_init (CockpitMetricsClass *klass)
{
  CockpitChannelClass *channel_class = COCKPIT_CHANNEL_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = cockpit_metrics_dispose;

  channel_class->recv = cockpit_metrics_recv;
  channel_class->close = cockpit_metrics_close;

  g_type_class_add_private (klass, sizeof (CockpitMetricsPrivate));
}

static gboolean
on_timeout_tick (gpointer data)
{
  CockpitMetrics *self = data;
  CockpitMetricsClass *klass;
  gint64 next_interval;

  if (self->priv->timeout > 0)
    {
      g_source_remove (self->priv->timeout);
      self->priv->timeout = 0;
    }

  klass = COCKPIT_METRICS_GET_CLASS (self);
  if (klass->tick)
    (klass->tick) (self, self->priv->next);

  self->priv->next += self->priv->interval;
  next_interval = self->priv->next - g_get_monotonic_time() / 1000;
  if (next_interval < 0)
    next_interval = 0;

  if (next_interval <= G_MAXUINT)
    self->priv->timeout = g_timeout_add (next_interval, on_timeout_tick, self);
  else if (next_interval / 1000 <= G_MAXUINT)
    self->priv->timeout = g_timeout_add_seconds (next_interval / 1000, on_timeout_tick, self);
  else
    cockpit_channel_close (COCKPIT_CHANNEL (self), "internal-error");

  return FALSE;
}

void
cockpit_metrics_metronome (CockpitMetrics *self,
                           gint64 interval)
{
  g_return_if_fail (self->priv->timeout == 0);
  g_return_if_fail (interval > 0);

  self->priv->next = g_get_monotonic_time() / 1000;
  self->priv->interval = interval;
  on_timeout_tick (self);
}

CockpitChannel *
cockpit_metrics_open (CockpitTransport *transport,
                      const gchar *id,
                      JsonObject *options)
{
  GType channel_type;
  const gchar *source;

    /* Source will be further validated when channel opens */
  if (!cockpit_json_get_string (options, "source", NULL, &source))
    source = NULL;

#if 0
  if (g_strcmp0 (source, "internal") == 0)
    channel_type = COCKPIT_TYPE_INTERNAL_METRICS;
  else
#endif
    channel_type = COCKPIT_TYPE_PCP_METRICS;

  return g_object_new (channel_type,
                       "transport", transport,
                       "id", id,
                       "options", options,
                       NULL);
}

/* ---- */

void
cockpit_compressed_array_builder_init (CockpitCompressedArrayBuilder *compr)
{
  compr->array = json_array_new ();
  compr->n_no_skip = 0;
  compr->n_skip = 0;
}

void
cockpit_compressed_array_builder_add (CockpitCompressedArrayBuilder *compr,
                                      JsonNode *element)
{
  if (element == NULL)
    {
      if (compr->n_no_skip > 0)
        {
          g_assert (compr->n_skip == 0);
          int pos = json_array_get_length (compr->array) - compr->n_no_skip - 1;
          json_array_set_int_element (compr->array, pos, compr->n_no_skip);
          compr->n_no_skip = 0;
        }
      compr->n_skip++;
    }
  else
    {
      if (compr->n_skip > 0)
        {
          g_assert (compr->n_no_skip == 0);
          json_array_add_int_element (compr->array, -compr->n_skip);
          compr->n_skip = 0;
          json_array_add_int_element (compr->array, 0);
        }
      json_array_add_element (compr->array, element);
      compr->n_no_skip++;
    }
}

JsonArray *
cockpit_compressed_array_builder_finish (CockpitCompressedArrayBuilder *compr)
{
  if (compr->n_no_skip > 0)
    {
      g_assert (compr->n_skip == 0);
      int pos = json_array_get_length (compr->array) - compr->n_no_skip - 1;
      json_array_set_int_element (compr->array, pos, compr->n_no_skip);
    }
  if (compr->n_skip > 0)
    {
      g_assert (compr->n_no_skip == 0);
      json_array_add_int_element (compr->array, -compr->n_skip);
    }

  return compr->array;
}

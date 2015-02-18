#!/bin/bash

<<<<<<< HEAD
#set -x
SCRIPT_DIR=`dirname "$(readlink -f "$0")"`
COCKPIT_DIR=$(cd $SCRIPT_DIR;cd ../..; echo `pwd`)
BASE=test/tests
source $SCRIPT_DIR/lib/host_setup.sh
source $SCRIPT_DIR/lib/create_test_machine.sh
LUSER=`whoami`
=======
unset command_not_found_handle
BASE_PCKGS="virt-deploy pystache sshpass telnet fabric"
GRP="virtualization"

function host_dependencies_fedora(){
    sudo yum -y yum-plugin-copr
    sudo yum -y copr enable fsimonce/virt-deploy
    sudo yum -y install $BASE_PCKGS
}

function host_dependencies_rhel7(){
    curl https://copr.fedoraproject.org/coprs/fsimonce/virt-deploy/repo/epel-7/fsimonce-virt-deploy-epel-7.repo > virt-deploy.repo
    sudo /bin/cp virt-deploy.repo /etc/yum.repos.d/
    sudo yum -y install $BASE_PCKGS
>>>>>>> 8ce60b9a4b516f4161ed95fd804d11279ac0e5d9

check_host $LUSER || sudo $SCRIPT_DIR/lib/host_setup.sh $LUSER || exit 1

<<<<<<< HEAD
# generate key for actual user if not generated yet
if [ ! -e ~/.ssh/id_rsa ]; then
    ssh-keygen -q -f ~/.ssh/id_rsa -N "" </dev/null
fi

PREFIX=checkmachine7
=======
function host_virtlibpolicy_solver(){
    LUSER=`whoami`
    sudo groupadd $GRP
    sudo usermod -a -G $GRP $LUSER
    newgrp $GRP
    sudo chgrp $GRP  /var/lib/libvirt/images
    sudo chmod g+rwx /var/lib/libvirt/images
    sudo tee /etc/polkit-1/localauthority/50-local.d/50-org.example-libvirt-remote-access.pkla <<< "[libvirt Management Access]
Identity=unix-group:$GRP
Action=org.libvirt.unix.manage
ResultAny=yes
ResultInactive=yes
ResultActive=yes"
}

if rpm -q $BASE_PCKGS >& /dev/null; then
    echo "All packages alread installed"
else
    if cat /etc/redhat-release | grep -sq "Red Hat"; then
        host_dependencies_rhel7
    elif cat /etc/redhat-release | grep -sq "Fedora"; then
        host_dependencies_fedora
    else
        echo "Now are supported only Fedora and Red Hat installation methods"
        exit 10
    fi
fi

if groups `whoami` | grep -s $GRP; then
    echo "Virtualization enabled for user"
else
    host_virtlibpolicy_solver
fi

# generate key if not generated yet
ssh-keygen -q -f ~/.ssh/id_rsa -N "" </dev/null

AVOCADO_NEW="https://github.com/avocado-framework/avocado/archive/master.zip"
AVOCADO="avocado-master"
AVOCADO_SOURCE="
(
 set -e;
 cd /tmp;
 curl -L $AVOCADO_NEW > $AVOCADO.zip;
 unzip -o $AVOCADO.zip;
 cd $AVOCADO;
 sudo ./setup.py install;
)
"
BASE=test/tests
source $BASE/lib/create_test_machine.sh
AVOCADO_TEST_DIR=/tmp/avocado-test/tests

PREFIX=checkmachine1
>>>>>>> 8ce60b9a4b516f4161ed95fd804d11279ac0e5d9
DISTRO=fedora-21
virt-create $PREFIX $DISTRO
NAME=`vm_get_name $PREFIX $DISTRO`
IP=`vm_get_ip $NAME`
PASSWD=`vm_get_pass $NAME`

avocado -v || sudo bash -c "`avocado_git_install`"
LOCAL_VERSION=`avocado -v 2>&1 |grep Avo`
vm_ssh $NAME avocado -v || vm_ssh $NAME "`avocado_git_install`"
REMOTE_VERSION=`vm_ssh $NAME "avocado -v " 2>&1 | grep Avo`
if [ "$LOCAL_VERSION" != "$REMOTE_VERSION" ]; then
    echo "avocado versions are not same on LOCAL and REMOTE $LOCAL_VERSION != $REMOTE_VERSION (SHOULD BE)"
    exit 11
fi

# workaound for slow snapshost creation (delete all existing snaps)
vm_delete_snaps $NAME

AVOCADO_TEST_DIR=/tmp/avocado-test
mkdir -p $AVOCADO_TEST_DIR
/bin/cp -r $COCKPIT_DIR/$BASE/* $AVOCADO_TEST_DIR
vm_ssh $NAME mkdir -p /root/avocado/tests/$AVOCADO_TEST_DIR /root/cockpit
( cd $COCKPIT_DIR; tar cf - . | vm_ssh $NAME tar xf - --directory /root/cockpit; )
vm_ssh $NAME cp -r /root/cockpit/$BASE/\* /root/avocado/tests$AVOCADO_TEST_DIR

AVOCADO_PARAMS="--vm-domain $NAME --vm-username root --vm-password $PASSWD --vm-hostname $IP"
<<<<<<< HEAD
avocado run $AVOCADO_PARAMS --xunit out1.xml $AVOCADO_TEST_DIR/{sources.sh,inittest.sh}
avocado run $AVOCADO_PARAMS --xunit out2.xml --vm-clean $AVOCADO_TEST_DIR/{compiletest.sh,checklogin.py}
=======
sudo avocado run $AVOCADO_PARAMS --xunit out1.xml $AVOCADO_TEST_DIR/{sources.sh,inittest.sh}
sudo avocado run $AVOCADO_PARAMS --xunit out2.xml  --vm-clean $AVOCADO_TEST_DIR/{compiletest.sh,checklogin.py}
>>>>>>> 8ce60b9a4b516f4161ed95fd804d11279ac0e5d9


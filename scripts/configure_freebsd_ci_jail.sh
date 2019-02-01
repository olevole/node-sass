#!/usr/bin/env sh

set -o xtrace

jailName=$1
skelDirectory=$2

echo "Installing build dependencies for cbsd"
pkg install -y libssh2 rsync sqlite3 git pkgconf

echo "Clone and setup cbsd"
git clone https://github.com/cbsd/cbsd.git /usr/local/cbsd --single-branch --branch develop --depth 1

cd /usr/local/etc/rc.d
ln -sf /usr/local/cbsd/rc.d/cbsdd
mkdir -p /usr/local/libexec/bsdconfig
cd /usr/local/libexec/bsdconfig
ln -s /usr/local/cbsd/share/bsdconfig/cbsd
pw useradd cbsd -s /bin/sh -d /nonexistent -c "cbsd user"

echo "Writing 'FreeBSD-bases' configuration file"
cd /tmp
ln -s /usr/local/cbsd/etc
cat > /tmp/etc/FreeBSD-bases.conf << EOF
auto_baseupdate=0
default_obtain_base_method="extract repo"
default_obtain_base_extract_source="/usr/freebsd-dist/base.txz"
default_obtain_base_repo_sources="https://bintray.com/am11/freebsd-dist/download_file?file_path=base-11.2-i386.txz"
EOF

# determine uplink ip address
# determine uplink iface
auto_iface=$( /sbin/route -n get 0.0.0.0 |/usr/bin/awk '/interface/{print $2}' )
my_ipv4=$( /sbin/ifconfig ${auto_iface} | /usr/bin/awk '/inet [0-9]+/{print $2}' )

if [ -z "${my_ipv4}" ]; then
	echo "IPv4 not detected"
	exit 1
fi

cbsd_workdir=/usr/jails

echo "Writing '${jailName}' configuration file"
cat > /tmp/${jailName}.jconf << EOF
jname="${jailName}"
path="${cbsd_workdir}/${jailName}"
host_hostname="${jailName}.my.domain"
ip4_addr="${my_ipv4}"
mount_devfs="1"
allow_mount="1"
allow_devfs="1"
allow_nullfs="1"
allow_raw_sockets="1"
mount_fstab="${cbsd_workdir}/jails-fstab/fstab.${jailName}"
arch="i386"
mkhostsfile="1"
devfs_ruleset="4"
ver="11.2"
basename=""
baserw="0"
mount_src="0"
mount_obj="0"
mount_kernel="0"
mount_ports="1"
astart="1"
data="${cbsd_workdir}/jails-data/${jailName}-data"
vnet="0"
applytpl="1"
mdsize="0"
rcconf="${cbsd_workdir}/jails-rcconf/rc.conf_${jailName}"
floatresolv="1"
exec_poststart="0"
exec_poststop=""
exec_prestart="0"
exec_prestop="0"
exec_master_poststart="0"
exec_master_poststop="0"
exec_master_prestart="0"
exec_master_prestop="0"
pkg_bootstrap="1"
interface="0"
jailskeldir="$skelDirectory"
exec_start="/bin/sh /etc/rc"
exec_stop="/bin/sh /etc/rc.shutdown"
runasap="1"	# run upon jail create
EOF

echo "Initializing cbsd environment"
env workdir=${cbsd_workdir} /usr/local/cbsd/sudoexec/initenv /usr/local/cbsd/share/initenv.conf

echo "Creating ${jailName}"
cbsd jcreate jconf=/tmp/${jailName}.jconf inter=0 arch=i386
cbsd jailscp /etc/resolv.conf ${jailName}:/etc/resolv.conf

echo "${jailName} created"
cd /etc/skel

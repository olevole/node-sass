#!/usr/bin/env sh

set -o xtrace

skelDirectory=$1

echo "Installing build dependencies for cbsd"
pkg install -y libssh2 rsync sqlite3 git pkgconf

echo "Clone and setup cbsd"
#git clone https://github.com/am11/cbsd.git /usr/local/cbsd --single-branch --branch develop --depth 1
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

cat /etc/rc.conf

ifconfig

echo "Writing 'jail-11i386' configuration file"
cat > /tmp/jail-11i386.jconf << EOF
jname="jail-11i386"
path="/usr/jails/jail-11i386"
host_hostname="jail-11i386.my.domain"
ip4_addr="DHCP"
mount_devfs="1"
allow_mount="1"
allow_devfs="1"
allow_nullfs="1"
allow_raw_sockets="1"
mount_fstab="/usr/jails/jails-fstab/fstab.jail-11i386"
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
data="/usr/jails/jails-data/jail-11i386-data"
vnet="0"
applytpl="1"
mdsize="0"
rcconf="/usr/jails/jails-rcconf/rc.conf_jail-11i386"
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
interface="auto"
jailskeldir="$skelDirectory"
exec_start="/bin/sh /etc/rc"
exec_stop="/bin/sh /etc/rc.shutdown"
EOF

echo "Initializing cbsd environment"

# redefine nodeippool
cp /usr/local/cbsd/share/initenv.conf /tmp/initenv.conf
sysrc -qf /tmp/initenv.conf nodeippool=192.168.0.0/24
# determine uplink iface
auto_iface=$( /sbin/route -n get 0.0.0.0 |/usr/bin/awk '/interface/{print $2}' )
workdir=/tmp /usr/local/cbsd/sudoexec/initenv /tmp/initenv.conf
cbsd natcfg fw_new=fw natip_new=${auto_iface}
kldload pf || true
cbsd naton
sysctl -w net.inet.ip.forwarding=1

echo "Creating jail-11i386"
cbsd jcreate jconf=/tmp/jail-11i386.jconf inter=0 arch=i386

# display the installed images
# cbsd bases display=ver,source

echo "Starting jail-11i386"
cbsd jstart jail-11i386

# debug
uname -a
cbsd jls
jls -v
cat /etc/resolv.conf
grep nodeip ~cbsd/nc.inventory
cbsd jexec jname=jail-11i386 cat /etc/resolv.conf
cbsd jailscp /etc/resolv.conf jail-11i386:/etc/resolv.conf
cbsd jexec jname=jail-11i386 ifconfig
ping -c1 8.8.8.8 ||true
#nslookup google.com 8.8.8.8 ||true
#dig @8.8.8.8 google.com ||true
nc -z google.com 80 ||true

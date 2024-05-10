#
# To run this test you need to add the following lines to /etc/sudoers.d/saunafstest:
#
# saunafstest ALL = NOPASSWD: /bin/mount, /bin/umount, /bin/pkill, /bin/mkdir, /bin/touch
# saunafstest ALL = NOPASSWD: /usr/bin/ganesha.nfsd
#
# The path for the Ganesha daemon should match the installation folder inside the test.
#
# Goal of test:
# Verify the performance of Ganesha and Linux clients when performing sequential
# reads with the fio tool.
#
assert_program_installed fio

timeout_set 45 seconds

CHUNKSERVERS=5 \
	USE_RAMDISK=YES \
	MOUNT_EXTRA_CONFIG="sfscachemode=NEVER" \
	CHUNKSERVER_EXTRA_CONFIG="READ_AHEAD_KB = 1024|MAX_READ_BEHIND_KB = 2048"
	setup_local_empty_saunafs info

test_error_cleanup() {
	cd ${TEMP_DIR}
	sudo umount -l ${TEMP_DIR}/mnt/ganesha
	sudo pkill -9 ganesha.nfsd
}

mkdir -p ${TEMP_DIR}/mnt/ganesha
mkdir -p ${info[mount0]}/linux

# Create PID file for Ganesha
PID_FILE=/var/run/ganesha/ganesha.pid
if [ ! -f ${PID_FILE} ]; then
	echo "ganesha.pid doesn't exists, creating it...";
	sudo mkdir -p /var/run/ganesha;
	sudo touch ${PID_FILE};
else
	echo "ganesha.pid already exists";
fi

cd ${info[mount0]}

cat <<EOF > ${info[mount0]}/ganesha.conf
NFSV4 {
	Grace_Period = 5;
}
EXPORT
{
	Attr_Expiration_Time = 0;
	Export_Id = 1;
	Path = /;
	Pseudo = /;
	Access_Type = RW;
	FSAL {
		Name = SaunaFS;
		hostname = localhost;
		port = ${saunafs_info_[matocl]};
		# How often to retry to connect
		io_retries = 5;
		cache_expiration_time_ms = 2500;
	}
	Protocols = 4;
	CLIENT {
		Clients = localhost;
	}
}
SaunaFS {
	PNFS_DS = true;
	PNFS_MDS = true;
}
EOF

sudo /usr/bin/ganesha.nfsd -f ${info[mount0]}/ganesha.conf

assert_eventually 'showmount -e localhost'
sudo mount -vvvv localhost:/ $TEMP_DIR/mnt/ganesha

echo ""
echo "Running fio sequential read on top of Ganesha Client:"
cd ${TEMP_DIR}/mnt/ganesha

# Run fio sequential read on top of Ganesha Client
ganesha_report="$(fio --name=sequential_read_ganesha \
  --directory=${TEMP_DIR}/mnt/ganesha --size=500M --rw=read --numjobs=1 \
  --ioengine=libaio --group_reporting --bs=4M --direct=1 --iodepth=4)"

# print Ganesha read statistics
echo "${ganesha_report}"

ganesha_read_speed="$(echo "${ganesha_report}" | grep "READ: bw=" | \
                      cut -d ' ' -f 10 | cut -c2- | sed s/'),'/''/g)"

echo ""
echo "Running fio sequential read on top of Linux Client:"
cd ${info[mount0]}

# Run fio sequential read on top of Linux Client
linuxClient_report="$(fio --name=name=sequential_read_linux \
  --directory=${info[mount0]}/linux --size=500M --rw=read --numjobs=1 \
  --ioengine=libaio --group_reporting --bs=4M --direct=1 --iodepth=4)"

# print Linux Client read statistics
echo "${linuxClient_report}"

linuxClient_read_speed="$(echo "${linuxClient_report}" | grep "READ: bw=" | \
                          cut -d ' ' -f 10 | cut -c2- | sed s/'),'/''/g)"

# Show performances of both clients
echo ""
echo " Operation: Ganesha Client ---- Linux Client ---"
echo -e " READ:      ${ganesha_read_speed}  \t\t ${linuxClient_read_speed}"

test_error_cleanup || true

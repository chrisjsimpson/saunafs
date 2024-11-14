timeout_set 5 minutes

CHUNKSERVERS=1 \
	MOUNT_EXTRA_CONFIG="sfscachemode=NEVER`
			`|cacheexpirationtime=10000`
			`|readcachemaxsizepercentage=1`
			`|sfschunkserverwavereadto=2000`
			`|sfsioretries=50`
			`|readaheadmaxwindowsize=5000`
			`|sfschunkservertotalreadto=8000" \
	setup_local_empty_saunafs info

cd ${info[mount0]}

five_percent_ram_mb=$(awk '/MemTotal/ {print int($2 / 1024 * 0.05)}' /proc/meminfo)
echo "five_percent_ram_mb: ${five_percent_ram_mb}"

fio --name=test_multiple_reads --directory=${info[mount0]} --size=${five_percent_ram_mb}M --rw=write --ioengine=libaio --group_reporting --numjobs=18 --bs=1M --direct=1 --iodepth=1
fio --name=test_multiple_reads --directory=${info[mount0]} --size=${five_percent_ram_mb}M --rw=read --ioengine=libaio --group_reporting --numjobs=18 --bs=1M --direct=1 --iodepth=1

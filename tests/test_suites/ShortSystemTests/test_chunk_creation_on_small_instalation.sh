timeout_set "30 seconds"
CHUNKSERVERS=5 \
	USE_RAMDISK=YES \
	MOUNT_EXTRA_CONFIG="mfscachemode=NEVER" \
	setup_local_empty_saunafs info

cd "${info[mount0]}"

mkdir dir_xor5
saunafs setgoal xor5 dir_xor5
for i in {1..60}; do
	FILE_SIZE=1M file-generate "dir_xor5/file$i"
done
mkdir dir_9
saunafs setgoal 9 dir_9
FILE_SIZE=1k file-generate dir_9/file

chunks=$(saunafs fileinfo */* | grep copy)
expect_equals 5 $(grep -v part <<< "$chunks" | grep -v parity | wc -l)

# No parity is expected to appear
for part in "part "{2..6}"/6 of xor5"; do
	count=$(grep -c "$part" <<< "$chunks")
	MESSAGE="There should be at least 30 '$part'" expect_less_or_equal 30 $count
done;
file-validate */*

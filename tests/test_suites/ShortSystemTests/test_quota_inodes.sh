USE_RAMDISK=YES \
	SFSEXPORTS_EXTRA_OPTIONS="allcanchangequota,ignoregid" \
	MOUNT_EXTRA_CONFIG="mfscachemode=NEVER" \
	setup_local_empty_saunafs info

cd "${info[mount0]}"

gid1=$(id -g saunafstest_1)
gid2=$(id -g saunafstest)

softlimit=3
hardlimit=14

saunafs setquota -g $gid1 0 0 $softlimit $hardlimit .

# exceed quota by creating 1 directory and some files (8 inodes in total):
sudo -nu saunafstest_1 mkdir dir_$gid1
for i in 2 3 4; do
	verify_quota "Group $gid1 -- 0 0 0 $((i-1)) $softlimit $hardlimit" saunafstest_1
	sudo -nu saunafstest_1 touch dir_$gid1/$i
done
for i in 5 6; do
	# after exceeding soft limit - changed into +:
	verify_quota "Group $gid1 -+ 0 0 0 $((i-1)) $softlimit $hardlimit" saunafstest_1
	sudo -nu saunafstest_1 touch dir_$gid1/$i
done

# soft links do affect usage and are checked against limits:
sudo -nu saunafstest_1 ln -s dir_$gid1/4 dir_$gid1/soft1
verify_quota "Group $gid1 -+ 0 0 0 7 $softlimit $hardlimit" saunafstest_1

# snapshots are allowed, if none of the uid/gid of files residing
# in a directory reached its limit:
sudo -nu saunafstest_1 $(which saunafs) makesnapshot dir_$gid1 snapshot
# sudo does not necessarily pass '$PATH', even if -E is used, that's
# why a workaround with 'which' was used above
verify_quota "Group $gid1 -+ 0 0 0 14 $softlimit $hardlimit" saunafstest_1

# check if quota can't be exceeded further:
expect_failure sudo -nu saunafstest_1 touch dir_$gid1/file
expect_failure sudo -nu saunafstest_1 mkdir dir2_$gid1
expect_failure sudo -nu saunafstest_1 ln -s dir_$gid1/4 dir_$gid1/soft2
expect_failure sudo -nu saunafstest_1 $(which saunafs) makesnapshot dir_$gid1 snapshot2
verify_quota "Group $gid1 -+ 0 0 0 14 $softlimit $hardlimit" saunafstest_1

# hard links don't affect usage and are not checked against limits:
sudo -nu saunafstest_1 ln dir_$gid1/4 hard
verify_quota "Group $gid1 -+ 0 0 0 14 $softlimit $hardlimit" saunafstest_1

# check if chgrp is properly handled
sudo -nu saunafstest_1 chgrp -R $gid2 dir_$gid1
verify_quota "Group $gid1 -+ 0 0 0 7 $softlimit $hardlimit" saunafstest_1
verify_quota "Group $gid2 -- 0 0 0 7 0 0" saunafstest
# check if quota can't be exceeded by one:
for i in {8..14}; do
	sudo -nu saunafstest_1 touch dir_$gid1/file$i
done
expect_failure sudo -nu saunafstest_1 touch dir_$gid1/file15
verify_quota "Group $gid1 -+ 0 0 0 $hardlimit $softlimit $hardlimit" saunafstest_1

#It would be nice to test chown as well, but I don't know how to do that without using superuser


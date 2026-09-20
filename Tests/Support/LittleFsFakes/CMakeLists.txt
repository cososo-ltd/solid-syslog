# LittleFsFakes - host-side fakes of the lfs_* API used by Tests/LittleFs.
#
# The real lfs.h comes from $LITTLEFS_PATH, so the adapter compiles against
# the same struct definitions an integrator gets and drift is a compile
# error. Only the functions are faked. Each test executable pulls the fake
# source in directly, the way Tests/FatFs does.

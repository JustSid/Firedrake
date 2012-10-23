if [ $# -eq 0 ]; then
    ssh justsid@192.168.16.152 "cd /mnt/hgfs/Firedrake/; make install; exit"
    exit 0
fi

if [ "$@" == "--clean" ]; then
	ssh justsid@192.168.16.152 "cd /mnt/hgfs/Firedrake/; make clean; exit"
	exit 0
fi

echo "No argument to build, --clean to clean all targets"!

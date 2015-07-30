This folder contains all projects and examples related to Dom0 for L4Re/Fiasco.OC.

These are the instructions to build and execute them.


Building:
0. Install all the required dependencies. See l4re.org or Attachment A of my Bachelor's Thesis (in German) for details.
In addition to the dependencies mentioned there, you will also need vde. On debian, the package is called "vde2".
1. Download l4re-snapshot-2014092821.tar.xz from http://l4re.org/download/snapshots-oc/l4re-snapshot-2014092821.tar.xz or http://home.in.tum.de/~stark/l4re-snapshot-2014092821.tar.xz
2. Open a terminal and change into the root directory of the project (where this README is).
3. Unpack the snapshot there:
# tar xf /path/to/snapshot-archive/l4re-snapshot-2014092821.tar.xz
The folder l4re-snapshot-2014092821 should then be in the same directory as the other folders (dom0-main, required_files etc.), this README and the Makefile.
4. Configure the snapshot:
# make -C l4re-snapshot-2014092821/ setup
Select x86-32 in the dialog.
5. Delete the ankh-examples because their build is broken in the current snapshot:
# rm -r l4re-snapshot-2014092821/src/l4/pkg/ankh/examples
6. Some files need to be copied into the l4re-tree (some of them replacing others) before building:
# cp -r required_files/l4re-snapshot-2014092821/* l4re-snapshot-2014092821/
7. Build Fiasco.OC and L4Re (optionally parallelized with -jX):
# make -C l4re-snapshot-2014092821/obj/fiasco/ia32/
# make -C l4re-snapshot-2014092821/obj/l4/x86/
8. The automatic ankh build is also broken in the current snapshot. Do it manually:
# make -C l4re-snapshot-2014092821/src/l4/pkg/ankh O=../../../../obj/l4/x86/
9. Build the external components:
# make O=l4re-snapshot-2014092821/obj/l4/x86/
The makefile then builds all (sub-)projects automatically.
Important: Do not parallelize this build with -jX,
so that dependencies can be built in the correct order.
(Reason: dependency handling is not available for external packages)

Testing: If no error occured, the testscripts can now be launched:
1. Start a vde_switch:
# sudo vde_switch -daemon -mod 660 -group users
(If this fails, try adding your user to the "users" group, then try again.)
2. Change into the L4 build directory:
# cd l4re-snapshot-2014092821/obj/l4/x86/
3. Execute one of the testscripts:
- For the filesystem-test (tmpfs/ libfs/ machine-global tmpfs):
# ./run-fs.sh
- For the simple network-test (1 echo-server and three clients):
# ./run-network.sh
- For the full-blown dom0-example (4 instances):
# ./run-dom0.sh

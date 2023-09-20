Build Instructions

1.	Download patch-elf repository from https://gitlab/AmpereComputing/patch-elf.

[root@sut01sys-b212 ]# git clone https://gitlab.com/amperecomputing/patch-elf <br />
warning: redirecting to https://gitlab.com/AmpereComputing/personal/steve.clevenger/patch-elf.git/ <br />
remote: Enumerating objects: 31, done. <br />
remote: Counting objects: 100% (31/31), done .<br />
remote: Compressing objects: 100% (30/30), done. <br />
remote: Total 31 (delta 9), reused 0 (delta 0), pack-reused 0 <br />
Receiving objects: 100% (31/31), 62.99 KiB | 21.00 MiB/s, done. <br />
Resolving deltas: 100% (9/9), done. <br />
[root@sut01sys-b212 ]# cd patch-elf <br />`

2.	Run the setup.sh script. This script clones the ELFIO dependencies.

[root@sut01sys-b212 patch-elf]# ./setup.sh <br />
hint: Using 'master' as the name for the initial branch. This default branch name<br />
hint: is subject to change. To configure the initial branch name to use in all<br />
hint: of your new repositories, which will suppress this warning, call:<br />
hint: <br />
hint: 	git config --global init.defaultBranch <name> <br />
hint: <br />
hint: Names commonly chosen instead of 'master' are 'main', 'trunk' and <br />
hint: 'development'. The just-created branch can be renamed via this command: <br />
hint: <br />
hint: 	git branch -m <name> <br />
Initialized empty Git repository in /home/stevec/tmp/patch-elf/ELFIO/.git/ <br />
remote: Enumerating objects: 243, done. <br />
remote: Counting objects: 100% (243/243), done. <br />
remote: Compressing objects: 100% (211/211), done. <br />
remote: Total 243 (delta 67), reused 149 (delta 29), pack-reused 0 <br />
Receiving objects: 100% (243/243), 3.95 MiB | 8.85 MiB/s, done. <br />
Resolving deltas: 100% (67/67), done. <br />
From https://github.com/serge1/ELFIO <br />
 * tag               Release_3.12 -> FETCH_HEAD <br />
.: <br />
total 432 <br />

3. Build the patch-elf executable

[root@sut01sys-b212 ]# ./build.sh <br />
[root@sut01sys-b212 ]# ls -l <br />
-rwxr-xr-x. 1 root root    139 Aug 30 16:49 build <br />
drwxr-xr-x. 4 root root    128 Aug 30 16:55 ELFIO <br />
-rw-r--r--. 1 root root   1525 Aug 30 16:49 LICENSE.txt <br />
-rwxr-xr-x. 1 root root 412096 Aug 30 16:55 patch-elf <br />
-rw-r--r--. 1 root root   9880 Aug 30 16:49 patch-elf.cpp <br />
-rwxr-xr-x. 1 root root    388 Aug 30 16:55 setup.sh <br />
[root@sut01sys-b212 patch-elf] # <br />


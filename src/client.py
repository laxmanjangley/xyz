import os
from glob import glob
from os import path

import shlex, time, logging, os, sys
from subprocess import Popen, PIPE, STDOUT

logger = logging.getLogger()
logger.setLevel(logging.INFO)
# hdlr = logging.FileHandler('/var/tmp/myapp.log')
# logger.addHandler(hdlr) 

def log_subprocess_output(pipe):
    for line in iter(pipe.readline, b''):
        logger.info(line)


def package(ext):
	command = ext
	print command
	logging.info(command)
	args = shlex.split(command)
	logging.info(args)
	pack = time.time()
	process = Popen(args, stdout=PIPE, stderr=STDOUT)
	#output =
	with process.stdout:
	    log_subprocess_output(process.stdout)
	#exitcode = process.wait()
	return process.wait()
	#return output
	

# sub_dirs=glob('*/')
cur_dir=os.getcwd()
# for dirs in sub_dirs:
# 	path_cre=cur_dir+"/"+dirs
# 	torrent_file=cur_dir+"/.."+"/tmp/"+dirs[:len(dirs)-1]+".torrent"
# 	print torrent_file
# 	package("./uri "+path_cre +" -o "+torrent_file)
# 	#os.system("./uri "+path +" -o "+torrent_file)


os.chdir("/home/next/Desktop/tmp")
print os.system("pwd")	

package("wget http://192.168.35.212:15255/boost_1_61_0.torrent")
# package("wget http://192.168.35.212:15232/c.torrent")
# package("wget http://192.168.35.212:15232/d.torrent")
cur_dir_sup= os.getcwd();
files = filter(path.isfile, os.listdir(cur_dir_sup))

os.chdir(cur_dir)

final_magnet=""
for i in files:
	print i
	final_magnet=final_magnet+" "+cur_dir_sup+"/"+i+" "+cur_dir
print final_magnet

package("./client_ "+final_magnet)


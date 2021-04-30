import subprocess
import sys
import shutil

def install(package):
    subprocess.check_call([sys.executable, "-m", "pip", "install", package])

install('pyyaml==5.3.0')
install('pathlib')

import yaml
import os
from pathlib import Path, PurePosixPath, WindowsPath
import sys


CC = '-gcc'
CXX = '-g++'
AS = '-as'
AR = '-ar'
OBJCOPY = '-objcopy'
OBJDUMP = '-objdump'
SIZE = '-size'
GDB = '-gdb'

C_COMPILERS = dict({CC: 'CMAKE_C_COMPILER', CXX: 'CMAKE_CXX_COMPILER'})

TOOLS = [AS, AR, OBJCOPY, OBJDUMP, SIZE, GDB]

def gen_aux_config_files():
    with open('configurations.yaml') as f:
        # use safe_load instead load
        dataMap = yaml.safe_load(f)

        if dataMap is None:
            raise Exception("Configuration file \"configurations.yaml\" is empty!")
        try:
            if os.path.exists("generated"):
                shutil.rmtree("generated")

            os.mkdir("generated")
        except OSError:
            print("Creation of the directory generated failed ")
        else:
            print("Successfully created the generated directory")

        cmake = open("generated/CMakeConfigFile.cmake", "w")
        batch = open("generated/CMDConfigFile.cmd", "w")
        bat_build = open("BUILD", 'w')
        clean = open("CLEAN", 'w')

        for key in dataMap:
            if sys.platform != 'win32':
                path = PurePosixPath(dataMap.get(key))

            if not os.path.isdir(str(path)):
                cmake.close()
                batch.close()
                clean.close()
                os.remove(cmake.name)
                os.remove(batch.name)
                os.remove(clean.name)
                raise Exception("Path does not exist or is not a directory: " + dataMap.get(key))

            cmake.write('SET(' + key + ' \"' + str(path).replace('\\', '/') + '\")')
            cmake.write('\n')
            batch.write('set ' + key + '=' + ' \"' + str(path) + '\"')
            batch.write('\n')

        ARM_TOOLS_DIR = dataMap.get('ARM_TOOLS_DIR')
        for compiler_executable_name in os.listdir(ARM_TOOLS_DIR):
            if sys.platform != 'win32':
                if compiler_executable_name.endswith(CC):
                    cmake.write('SET(EABI_PREFIX \"' + compiler_executable_name[0:-4] + '\")')
                    cmake.write('\n')
                    batch.write('set EABI_PREFIX=' + ' \"' + compiler_executable_name[0:-4] + '\"')
                    batch.write('\n')
                    break


        if sys.platform != 'win32':
            bat_build.write("rm -rf cmake-build-debug")
            bat_build.write("\n")
            bat_build.write("rm -rf bin")
            bat_build.write("\n")
            bat_build.write("mkdir cmake-build-debug")
            bat_build.write("\n")
            bat_build.write("cd cmake-build-debug")
            bat_build.write("\n")
            bat_build.write("cmake ..")
            bat_build.write("\n")
            bat_build.write("make -j8")
            bat_build.write("\n")
            bat_build.write("cd ..")

            clean.write("rm -rf cmake-build-debug")
            clean.write("\n")
            clean.write("rm -rf bin")
        
        os.system("chmod +x BUILD")
        os.system("chmod +x CLEAN")




if __name__ == '__main__':
    gen_aux_config_files()

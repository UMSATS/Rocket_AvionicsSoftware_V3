import subprocess
import sys
import shutil

def install(package):
    subprocess.check_call([sys.executable, "-m", "pip", "install", package])

install('pyyaml==5.3.0')

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
        bat_build = open("BUILD.BAT", 'w')
        clean = open("CLEAN.BAT", 'w')

        for key in dataMap:
            if sys.platform == 'win32':
                path = WindowsPath(dataMap.get(key))
            else:
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
            if sys.platform == 'win32':
                if compiler_executable_name.find('eabi') != -1 and compiler_executable_name.endswith(CC + '.exe'):
                    cmake.write('SET(EABI_PREFIX \"' + compiler_executable_name[0:-8] + '\")')
                    cmake.write('\n')
                    batch.write('set EABI_PREFIX=' + ' \"' + compiler_executable_name[0:-8] + '\"')
                    batch.write('\n')
                    break
            else:
                if compiler_executable_name.endswith(CC):
                    cmake.write('SET(EABI_PREFIX \"' + compiler_executable_name[0:-4] + '\")')
                    cmake.write('\n')
                    batch.write('set EABI_PREFIX=' + ' \"' + compiler_executable_name[0:-4] + '\"')
                    batch.write('\n')
                    break

        if sys.platform != 'win32':
            return


        bat_build.write("@ECHO OFF")
        bat_build.write("\n")
        bat_build.write("rd /s /q \"cmake-build-debug\" > nul 2>&1")
        bat_build.write("\n")
        bat_build.write("rd /s /q \"bin\" > nul 2>&1")
        bat_build.write("\n")
        bat_build.write("mkdir cmake-build-debug")
        bat_build.write("\n")
        bat_build.write("cd cmake-build-debug")
        bat_build.write("\n")
        bat_build.write(str(Path(dataMap.get("CLION_CMAKE_PATH"))) + "\cmake .. -G \"CodeBlocks - MinGW Makefiles\"")
        bat_build.write("\n")
        bat_build.write(str(Path(dataMap.get("MINGW_ENV_PATH"))) + "\mingw32-make -j8")
        bat_build.write("\n")
        bat_build.write("cd ..")

        clean.write("@ECHO OFF")
        clean.write("\n")
        clean.write("rd /s /q \"cmake-build-debug\" > nul 2>&1")
        clean.write("\n")
        clean.write("rd /s /q \"bin\" > nul 2>&1")
        clean.write("\n")
        clean.write("rd /s /q \"generated\" > nul 2>&1")





if __name__ == '__main__':
    gen_aux_config_files()
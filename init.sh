#!/bin/bash


file_path="$( cd "$(dirname $0)" && pwd)"              #文件所在路径
root_path="$( cd "${file_path}" && cd .. && cd .. && pwd)"

#修改plugins/CMakeLists.txt以增加mysql_plugin插件的编译
message='add_subdirectory(mysql_plugin)'
file_name="${root_path}/plugins/CMakeLists.txt"
message_num=`grep "${message}" ${file_name} | wc -l`
if [ ${message_num} -eq 0 ];then
    echo "" >> $file_name
    echo "${message}" >> $file_name
    echo "add ${message} to ${file_name} finish"
else
    echo "${message} already exist in ${file_name}"
fi

#修改programs/nodeos/CMakeLists.txt已将插件连接到nodeos 
message='target_link_libraries( nodeos PRIVATE -Wl,${whole_archive_flag} mysql_plugin -Wl,${no_whole_archive_flag} )'
file_name="${root_path}/programs/nodeos/CMakeLists.txt"
message_num=`grep "${message}" ${file_name} | wc -l`
if [ ${message_num} -eq 0 ];then
    echo "" >> $file_name
    echo "${message}" >> $file_name
    echo "add ${message} to ${file_name} finish"
else
    echo "${message} already exist in ${file_name}"
fi

echo "init finish"

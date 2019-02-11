该插件是几年前随便写的，后面有空我会着手写一个限速的脚本，不再需要编译过程，敬请期待吧！

# bwlimitplugin
a plugin for openvpn,which can config bandwith limit for every vpn uer 

1、该功能由插件（编译完成后是一个so文件，将其放到对应的目录后再openvpn的配置文件中指定路径）实现，插件主要实现由服务器端到客户端的下行限速（实际公网环境，下行限速已足够，不需要上行限速，客户端的上行带宽是很低的）；

2、如果启用该功能，需要在openvpn的主配置文件（例如：vpn1.conf）中添加如下行，引号中的部分为插件的配置文件路径：

plugin /usr/local/openvpn-2.2.3/lib/bwlimitplugin.so "/usr/local/openvpn-2.2.3/etc/bwlimitplugin.cnf"


3、需要修改并推送插件的配置文件到服务器，路径为：/usr/local/openvpn-2.2.3/etc/bwlimitplugin.cnf。插件配置实例如下：

[root@VPN-server-Xen etc]# cat bwlimitplugin.cnf 

#this is the config file for bwlimitplugin.so

#default  means all other clients whose cname not specified in the followed list

default 2048

#config the total rate for tun,which unit is mbps 
total   1000

#bw limit list
#user cname  bw(kbps)

user abc      100
user xyz      1000
user test      1560
user test1     1234

说明：限速的配置主要有三个部分，总带宽、指定用户限速带宽、默认用户限速带宽
total关键字后面的数字配置   服务器下行总带宽（单位为Mbps，默认为100，千兆网卡通常配置为1000）；
default关键字后面的数字配置  未特殊指定的用户的下行带宽（单位为Kbps，默认为2048Kbps）；
user关键字后面接用户名和该用户的带宽，例如上例中指定了4个用户，其中用户名为：abc的用户带宽为100Kbps；

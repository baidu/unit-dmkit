# DM Kit 常见问题

## 编译brpc失败

参考BRPC官方文档 <https://github.com/brpc/brpc/>，检查是否已安装所需依赖库。建议使用系统Ubuntu 16.04或者CentOS 7。更多BRPC相关问题请在BRPC github库提起issue。

## 返回错误信息 DM policy resolve failed

DM Kit通过UNIT云端技能解析出的用户query的意图和词槽之后，需要根据对话意图结合当前对话状态在DM Kit配置中对应的policy处理对话流程。当用户query解析出的意图结合当前对话状态未能找到可选的policy时，DM Kit返回错误信息DM policy resolve failed。开发者需要检查：1)当前技能配置是否在products.json文件中进行注册；2）当前query解析结果意图在技能配置中是否配置了policy，详细配置说明参考[DM Kit快速上手](tutorial.md)。

## 返回错误信息 Failed to call unit bot api

DM Kit访问UNIT云端失败。具体原因需要查看DM Kit服务日志，常见原因是请求超时。对于请求超时的情况，先检查DM Kit所在服务器网络连接云端（默认地址为aip.baidubce.com）是否畅通。如果连接没有问题，则尝试切换请求client：DM Kit默认使用BRPC client请求UNIT云端，目前发现偶然情况下HTTPS访问云端出现卡死而返回超时错误。DM Kit支持切换为curl方式访问云端，将remote_services.json配置中client值由brpc修改为curl即可。需要注意使用curl方式时，建议升级openssl版本不低于1.1.0，libcurl版本不低于7.32。

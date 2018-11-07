# DM Kit 常见问题

## 编译brpc失败

参考BRPC官方文档 <https://github.com/brpc/brpc/>，检查是否已安装所需依赖库。建议使用系统Ubuntu 16.04或者CentOS 7。更多BRPC相关问题请在BRPC github库提起issue。

## 返回错误信息 DM policy resolve failed

DM Kit通过UNIT云端技能解析出的用户query的意图和词槽之后，需要根据对话意图结合当前对话状态在DM Kit配置中对应的policy处理对话流程。当用户query解析出的意图结合当前对话状态未能找到可选的policy或者选中policy执行流程出错时，DM Kit返回错误信息DM policy resolve failed。开发者需要检查：1)当前技能配置是否在products.json文件中进行注册；2）当前query解析结果意图在技能配置中是否配置了policy，详细配置说明参考[DM Kit快速上手](tutorial.md)；3）检查DM Kit日志查看policy执行流程是否出错，。

## 返回错误信息 Failed to call unit bot api

DM Kit访问UNIT云端失败。具体原因需要查看DM Kit服务日志，常见原因是请求超时。对于请求超时的情况，先检查DM Kit所在服务器网络连接云端（默认地址为 aip.baidubce.com）是否畅通。如果连接没有问题，则尝试切换请求client：DM Kit默认使用BRPC client请求UNIT云端，目前发现偶然情况下HTTPS访问云端出现卡死而返回超时错误。DM Kit支持切换为curl方式访问云端，将remote_services.json配置中client值由brpc修改为curl即可。需要注意使用curl方式时，建议升级openssl版本不低于1.1.0，libcurl版本不低于7.32。

## 返回错误信息 Unsupported action type satisfy

使用DM Kit需要将UNIT平台中【技能设置->高级设置】中【对话回应设置】一项设置为『使用DM Kit配置』。设置该选项之后，UNIT云端使用DM Kit支持的数据协议。如设置为『在UNIT平台上配置』, DM Kit无法识别UNIT云端数据协议，将返回错误Unsupported action type satisfy。

## DM Kit如何支持FAQ问答对

目前UNIT平台中将【对话回应】设置为【使用DM Kit配置】之后，如果对话触发了平台配置的FAQ问答集后，平台返回结果并不直接讲问答回复赋值给response中的say字段，但是开发者可以通过名为qid的词槽值中。因此，结合DM Kit配置可以从词槽qid解析出问答回复后进行返回。例如，平台创建问题意图FAQ_HELLO之后，可以在DM Kit对应技能的policy配置中添加一下policy支持FAQ_HELLO问答意图下的所有问答集：

```json
{
  "trigger": {
    "intent": "FAQ_HELLO",
    "slots": [],
    "state": ""
  },
  "params": [
    {
      "name": "answer_list",
      "type": "slot_val",
      "value": "qid"
    },
    {
      "name": "faq_answer",
      "type": "func_val",
      "value": "split_and_choose:{%answer_list%},|,random"
    }
  ],
  "output": [
    {
      "assertion": [],
      "session": {
        "context": {},
        "state": "001"
      },
      "result": [
        {
          "type": "tts",
          "value": "{%answer%}"
        }
      ]
    }
  ]
}
```

# DM Kit示例场景

## 查询流量及续订

该场景实现一个简单的手机流量查询及续订流量包的技能。

### 查询流量及续订 - UNIT平台配置

一共分为以下几个步骤：

* 创建技能
* 配置意图
* 配置词槽
* 添加词槽的词典值
* 添加特征词列表
* 导入对话模板

#### 创建BOT

1. 进入[百度理解与交互(unit)平台](http://ai.baidu.com/unit/v2#/sceneliblist)
2. 新建一个技能给查询流量及续订的demo使用。

#### 配置意图

1. 点击进入技能，新建对话意图，以INTENT_CHECK_DATA_USAGE为例。
2. 意图名称填写INTENT_CHECK_DATA_USAGE。
3. 意图别名可以根据自己偏好填写，比如填写查询手机流量。
4. 添加词槽。INTENT_CHECK_DATA_USAGE 所需的词槽为 user_time;
5. 词槽别名可以根据自己偏好填写，比如填写查询时间，查询月份等。
6. 添加完词槽名和词槽别名以后，点击下一步，进入到选择词典环节，user_time选择系统词槽词典中的 **sys_time(时间)** 词槽。
7. 点击下一步，词槽必填选项选择非必填，点击确认。

** 其他两个user_package_type和user_package_name的词槽使用自定义词典，将下方词槽对应的词典内容自行粘贴到txt中上传即可**

全部意图包括列表如下：

* INTENT_CHECK_DATA_USAGE，所需词槽为 user_time
* INTENT_BOOK_DATA_PACKAGE，所需词槽为 user_package_type，user_package_name
* INTENT_YES
* INTENT_NO
* INTENT_REPEAT
* INTENT_WAIT
* INTENT_CONTINUE

词槽列表：

* user_time，使用系统时间词槽

* user_package_type
  
    ```text
    省内
    #省内流量
    全国
    #全国流量
    夜间
    #夜间流量
    ```
* user_package_name
    ```text
    10元100M
    #10元100兆
    #十元一百M
    #十元一百兆
    20元300M
    #20元300兆
    #二十元三百M
    #二十元三百兆
    50元1G
    #五十元1G
    ```

#### 新增特征词

点击**效果优化**中的**对话模板**，选择下方的新建特征词，将名称和词典值依次填入。eg：kw_check即为名称，名称下方为词典值，直接复制粘贴即可。

特征词列表如下：

* kw_check
    ```text
    查一查
    查询
    ```
* kw_data_usage
    ```text
    流量
    流量情况
    流量使用情况
    流量使用
    ```
* kw_book
    ```text
    续定
    续订
    预定
    预订
    ```
* kw_package
    ```text
    流量包
    流量套餐
    ```
* kw_yes
    ```text
    是
    好
    对
    想
    要
    是的
    好的
    对的
    我想
    我要
    可以
    行的
    需要
    没问题
    ```
* kw_no
    ```text
    不
    不想
    不要
    不行
    别
    没有
    没了
    没
    不用
    不需要
    没有了
    ```
* kw_repeat
    ```text
    没听清楚
    再说一次
    ```
* kw_wait
    ```text
    稍等
    等等
    稍等一下
    等一下
    等一等
    ```
* kw_continue
    ```text
    你继续
    您继续
    继续
    ```

#### 导入对话模板

* 完成以上步骤后，再进行该步骤，不然系统会报错
* 将文件[demo_cellular_data_pattern.txt](demo_cellular_data_pattern.txt)下载导入即可

### 查询流量及续订 - DM Kit配置

该场景DM Kit配置为conf/app/demo/cellular_data.json文件，该文件由同目录下对应的.xml文件生成，可以将.xml文件在 <https://www.draw.io> 中导入查看。

## 调整银行卡额度

该场景实现一个简单的银行卡固定额度及临时额度调整的技能。

### 调整银行卡额度 - UNIT平台配置

平台配置参考查询流量的配置，所需的配置内容见下图。

所需意图包括列表：

* INTENT_ADJUST_QUOTA, 所需词槽为user_type, user_method, user_amount
* INTENT_YES
* INTENT_NO

词槽列表：

* user_amount, 复用系统sys_num词槽

* user_type
    ```text
    固定
    #固定额度
    临时
    #临时额度

    ```

* user_method
    ```text
    提升
    #提高
    #增加
    降低
    #减少
    #下调
    ```

特征词列表：

* kw_adjust
    ```text
    调整
    调整一下
    改变
    ```
* kw_quota
    ```text
    额度
    银行卡额度
    信用卡额度
    ```

* kw_yes
    ```text
    是
    好
    对
    想
    要
    是的
    好的
    对的
    我想
    我要
    可以
    行的
    没问题
    ```
* kw_no
    ```text
    不
    不想
    不要
    不对
    错了
    不是
    别
    没有
    没了
    没
    不用
    没有了
    ```

对话模板：

* 将文件[demo_quota_adjust_pattern.txt](demo_quota_adjust_pattern.txt)下载导入即可

### 调整银行卡额度 - DM Kit配置

该场景DM Kit配置为conf/app/demo/quota_adjust.json文件，该文件由同目录下对应的.xml文件生成，可以将.xml文件在 <https://www.draw.io> 中导入查看。

## 预订酒店

该场景实现一个简单的预订酒店的技能。

### 预订酒店 - UNIT平台配置

平台配置参考查询流量的配置，所需的配置内容见下图。

所需意图包括列表：

* INTENT_BOOK_HOTEL, 所需词槽为user_time, user_room_type, user_location, user_hotel
* INTENT_YES
* INTENT_NO

词槽列表：

* user_time, 复用系统sys_time词槽

* user_room_type
    ```text
    标间
    大床房
    单人间
    双人间
    双床房
    标准房
    标准间
    ```

* user_hotel, 复用系统sys_loc_hotel词槽

* user_hotel, 复用系统sys_loc词槽

特征词列表：

* kw_book
    ```text
    定
    预定
    订
    预订
    ```
* kw_hotel
    ```text
    旅馆
    酒店
    ```

* kw_yes
    ```text
    是
    好
    对
    想
    要
    是的
    好的
    对的
    我想
    我要
    可以
    行的
    没问题
    确认
    ```

* kw_no
    ```text
    不
    不想
    不要
    不对
    不是
    不用
    不行
    不可以
    别
    没
    没有
    没了
    没有了
    错了
    ```

对话模板：

* 将文件[demo_book_hotel_pattern.txt](demo_book_hotel_pattern.txt)下载导入即可

### 预订酒店 - DM Kit配置

该场景DM Kit配置为conf/app/demo/book_hotel.json文件，该文件由同目录下对应的.xml文件生成，可以将.xml文件在 <https://www.draw.io> 中导入查看。

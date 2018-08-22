# 对话流可视化配置

针对状态繁多、跳转复杂的垂类，DM Kit支持通过可视化编辑工具进行状态跳转流程的编辑设计，并同步转化为对话基础配置供对话管理引擎加载执行。

## 基于开源可视化工具的配置样例

这里的可视化编辑工具使用开源的[mxgraph](https://github.com/jgraph/mxgraph)可视化库，对话开发者可在可视化工具上进行图编辑，而该可视化库支持从图转化为xml文件，我们再利用转换框架实现对应的编译器将xml文件转化为对话基础配置加载执行。以demo场景【查询流量及续订】为例，步骤为：

* 在[draw.io](https://www.draw.io/)中按照[编辑规则](#编辑规则)进行图编辑
* 在编辑好的图导出为xml文件，放置于conf/app/demo目录下
* 运行language_compiler/run.py程序，该程序调用对应的转换器将conf/app/demo目录下的xml文件转化为json文件
* 将json文件注册于conf/app/products.json文件后，运行DMKit加载执行

### 编辑规则

规定使用以下构图元素进行编辑：

* 单箭头连线，单箭头连线是路程图中最基本的元素之一，用来表示状态的跳转。注意在使用连线的时候，连线的两端需要出现蓝色的 x 标识，以确保这个连线成功连接了两个框。
* 椭圆，用户节点，椭圆中存放的是用户的意图，以及槽位值（可选），内部语言格式为：
    ```text
    INTENT: intent_xxx
    SLOT:user_a,user_b
    ```
该节点表示用户输入query的NLU解析结果，结合指向该节点的BOT节点，构成了DM Kit基础配置中一个完成trigger条件

* 圆角矩形，BOT节点，圆角矩形中存放的是BOT的回复，内部格式为：
    ```text
    PARAM:param_type:param_name1=param_value2
    PARAM:param_type:param_name1=param_value2
    BOT:XXXXXXX{%param_name1%}XXX{%param_name2%}
    ```
该节点表示BOT应该执行的回复，同时节点中可以定义参数并对回复进行模板填充。

* 菱形，条件节点，在节点中可定义需要进行判断的变量:
    ```text
    PARAM:param_type:param_name1=param_value2
    PARAM:param_type:param_name1=param_value2
    ```
    同时对该节点连出的单箭头连线可以添加描述跳转条件，条件可使用在菱形中定义的变量，例如：
    ```text
    ge:{%param1%},1
    ```
    跳转条件描述中，可用&&或||来连接多个条件以表达“和”或“或”，例如：
    ```text
    ge:{%param1%},1 || eq: {%param1%}, 10
    ```
    ||和&&不可同时出现在一个条件描述中。

另外规定：

* 一个椭圆仅可以连向一个圆角矩形或者一个菱形
* 一个圆角矩形可以连向多个椭圆
* 一个菱形可以连向多个圆角矩形

详细使用示例参考conf/app/demo目录下demo场景的xml文件，该xml文件可在[draw.io](https://www.draw.io/)中导入查看

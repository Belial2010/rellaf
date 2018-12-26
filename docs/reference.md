# Rellaf 参考手册

## Model(反射类)
**头文件:** `model.h` 

**说明:**  
`Model`是`Rellaf`的核心，所有工作都围绕在`Model`反射类的特性来实现。

### 数据类型
`rellaf`反射类定义的类型如下:  

| rellaf类型 | c++类型 | 定义宏后缀 |
| ---------- | ---------- | --------- |
| no(表示未定义) | N/A | N/A |
| CHAR | char | char |
| INT16 | int16_t | int16 |
| INT | int | int |
| INT64 | int64_t | int64 |
| UINT16 | uint16_t | uint16 |
| UINT32 | uint32_t | uint32 |
| UINT64 | uint64_t | uint64 |
| BOOL | bool | bool |
| FLOAT | float | float |
| DOUBLE | double | double |
| STR | std::string | str |
| OBJECT | rellaf::Object | object |
| LIST | rellaf::List | list |
为什么是`int`, 而不是`int32`? 因为int这个类型广泛通用，早已得到普遍接受。

### Model基类
 
**说明:**   
`Model`是`rellaf`反射类的虚基类，用来定义通用接口。

**方法列表:**   

| 方法名 | 说明 | 返回值 | 参数 |
| ----- | --- | ------ | ------ |
| **rellaf_type** | 返回Model类型 | ModelType | N/A |
| **rellaf_name** | 返回Model名字 | std::string | N/A |
| **\<T\>tag** | 给当前Model设置一个枚举类型tag | T&(当前Model的引用) | EnumItem |
| **rellaf_tag** | 返回Model的tag | EnumItem | N/A |
| **create** | 构造一个新的Model | Model* | N/A |
| **clone** | 复制当前对象 | Model* | N/A |
| **assign** | 对象赋值 | void | N/A |
| **clear** | 清空 | void | N/A |
| **debug_str** | 返回debug字符串 | std::string | N/A |

### Plain
**说明:**   
继承`Model`，`Plain`是`rellaf`反射类对于C++普通类型的包装，模板类，需要指定包装类型，支持能够调用`std::string`的类型。

`Plain`在构造的时候，会自动检测包装的类型，并设置关联`ModelType`，从而`rellaf_type`能够将其返回。

**方法列表:**  

| 方法名 | 说明 | 返回值 | 参数 |
| ----- | --- | ------ | ------ |
| **\<T\>构造函数** | 默认构造，其他`Plain`对象或具体类型的赋值构造，复制构造和相应的移动构造 | N/A | `Plain<T>` |
| **Model基类方法** | 见`Model` | 见`Model` | 见`Model` |
| **value** | 取值 | 当前包装类型 | N/A |
| **set** | 赋值 | void | 当前包装类型 |
| **set_parse** | 从字符串parse成当前类型的值 | void | std::string |
| **equal_parse** | 从字符串parse成当前类型的值, 并做等值比较 | bool | std::string |
| **str** | 把当前类型的值转换成字符串表示 | std::string | N/A |

**例子:**  
```C++
Plain<int> intval = 233;
intval.str();               // "233"
intval.value();             // 233
intval.set(333);               
intval.value();             // 333
intval.set_parse("555");               
intval.value();             // 555
if (intval.equal_parse("555")) {
    // true
}
```

### List
**说明:**   
继承`Model`，表示列表。加入到`List`的`Model`对调用`clone`方法进行对象复制，内部会维护这些对象的生命周期，无需用户手动释放内存。

目前使用`std::deque`实现。

**方法列表:**  

| 方法名 | 说明 | 返回值 | 参数 |
| ----- | --- | ------ | ------ |
| **\<T\>构造函数** | 默认构造，赋值构造，复制构造和相应的移动构造 | N/A | `Plain<T>` |
| **Model基类方法** | 见`Model` | 见`Model` | 见`Model` |
| **size** | 长度 | size_t | N/A  |
| **empty** | 是否空 | bool | N/A |
| **push_front** | 头部插入 | void | Model* |
| **push_back** | 尾部插入 | void | Model* |
| **pop_front** | 头部删除 | void | N/A |
| **pop_back** | 尾部删除 | void | N/A |
| **\<T\>front** | 头部成员 | T* | N/A |
| **\<T\>back** | 尾部成员 | T* | N/A |
| **\<T\>at** | 指定索引成员 | T* | size_t 索引值 |
| **operator[]** | 指定索引成员 | Model* | size_t 索引值 |
| **set** | 设置指定索引成员 | void | Model* |
| **begin** | 起始迭代器 | std::deque<Model*>::const_iterator | N/A |
| **end** | 结束迭代器 | std::deque<Model*>::const_iterator | N/A |

**例子:** 
```C++
List list;
size_t size = list.size();      // 返回 0
bool is_empty = list.empty();   // 返回 true

// 插入成员，会进行内存复制，生命周期由Model管理。
Plain<int> item = 222;
list.push_front(&item);
list.push_back(&item);
size = list.size();             // 返回 2
is_empty = list.empty();        // 返回 false

// 索引数组成员，注意返回的是 Model*, 转换成具体类型即可
Plain<int>* ptr = list.front<Plain<int>>();
ptr = list.back<Plain<int>>();
ptr = (Plain<int>*)list[0];
ptr = (Plain<int>*)list[1];

// 遍历
for (Model* item : list) {  // 支持C++11 for循环语法
    // do something
}
for (auto i = list.begin(); i != list.end(); ++i) {
    // do something
}

// 修改
Plain<int> item_to_mod = 222;
list.set(0, &item_to_mod);
list.set(1, &item_to_mod);

// 弹出
list.pop_front();
list.pop_back();

// 清空
list.clear();
```

### Object
**说明:**   
继承`Model`，表示对象类型。加入到`Object`的`Model`对调用`clone`方法进行对象复制，内部会维护这些对象的生命周期，无需用户手动释放内存。

`Object`是`rellaf`的"特色"类型，需要自定义一个类型，然后继承Object类，然后配合宏来使用。例如：
```C++
class User : public Object {
rellaf_model_dcl(User);
}
rellaf_model_def(User);
```

**相关宏列表:**   

| 宏名 | 说明 | 参数 |
| ----- | --- | ------ | 
| rellaf_model_dcl | 申明`Object`对象 | 自定义类名 | 
| rellaf_model_def | 定义`Object`对象 | 自定义类名 | 
| rellaf_model_def_char | 定义`char`成员 | 字段名, 默认值 |
| rellaf_model_def_int16 | 定义`int16`成员 | 字段名, 默认值 |
| rellaf_model_def_int | 定义`int`成员 | 字段名, 默认值 |
| rellaf_model_def_int64 | 定义`int64`成员 | 字段名, 默认值 |
| rellaf_model_def_uint16 | 定义`uint16`成员 | 字段名, 默认值 |
| rellaf_model_def_uint32 | 定义`uint32`成员 | 字段名, 默认值 |
| rellaf_model_def_uint64 | 定义`uint64`成员 | 字段名, 默认值 |
| rellaf_model_def_bool | 定义`bool`成员 | 字段名, 默认值 |
| rellaf_model_def_float | 定义`float`成员 | 字段名, 默认值 |
| rellaf_model_def_double | 定义`double`成员 | 字段名, 默认值 |
| rellaf_model_def_str | 定义`str`成员 | 字段名, 默认值 |
| rellaf_model_def_object | 定义`Object`成员 | 子`Object`类名 |
| rellaf_model_def_list | 定义`List`成员 | 子`List`类名 |

**方法列表:**  

| 方法名 | 说明 | 返回值 | 参数 |
| ----- | ---- | ------ | ------ |
| **\<T\>构造函数** | 默认构造，赋值构造，复制构造和相应的移动构造 | N/A | `Plain<T>` |
| **Model基类方法** | 见`Model` | 见`Model` | 见`Model` |
| ***field*** | 字段取值 | void | 具体类型的字段值 |
| **set_*field*** | 设置字段值 | 具体类型的字段值 | N/A |
| ***field*_default** | 获得字段默认值 | 具体类型的字段值 | N/A |
| **is_plain_member** | 是否是plain类型成员 | bool | std::string 字段名 |
| **set_plain** | 设置普通字段 | void | std::string 字段名; std::string 字符串表示的字段值 |
| **\<T\>get_plain** | 获得普通字段 | Plain\<T\>*, 不存在返回nullptr | std::string 字段名 |
| **get_plains** | 获得普通字段集合 | std::map<std::string, Model*>&  | N/A |
| **is_object_member** | 是否是对象字段 | bool | std::string 字段名 |
| **get_object** | 获得对象字段 | Object* | std::string 字段名 |
| **get_objects** | 获得对象字段集合 | std::map<std::string, Object*>& | N/A |
| **is_list_member** | 是否是数组字段 | bool | std::string 字段名 |
| **get_list** | 获得数组字段 | List& | std::string 字段名 |
| **get_lists** | 获得数组字段集合 | std::map<std::string, List>& | N/A |

**例子:** 
定义
```c++
// 子对象
class SubObject : public Object {
rellaf_model_dcl(SubObject)
rellaf_model_def_uint16(port, 18765);
};
rellaf_model_def(SubObject);

// 父对象
class Obj : public Object {
rellaf_model_dcl(Obj)
rellaf_model_def_int(id, -111);
rellaf_model_def_str(name, "aaa");

// 定义对象，字段名，类型名（必须是Model类）
rellaf_model_def_object(sub, SubObject);

// 定义数组，字段名，类型名（必须是Model类）
rellaf_model_def_list(list, Plain<int>); 
};
rellaf_model_def(Obj);
```

操作
```c++
Obj object;
// 常规操作
object.set_id(123);
object.set_name("fankux");

// 初始情况下是nullptr
SubObject* ptr = object.sub();    

// 赋值一个对象, 会进行内存复制，生命周期由Model管理。
SubObject sub;
object.set_sub(&sub);

// 给对象成员的成员赋值(注意，对象成员方法返回的是指针)
object.sub()->set_port(8121);
// 通过字段名字符串索引, 若不存在则返回nullptr
ptr = object->get_object("sub");
ptr->set_port(8121);

// 取值, 返回 8121
uint16_t port = object.sub()->port();
port = object.sub()->get_uint16("port");
```


### Avoid
**说明:**  
继承`Model`，表示`无类型`。

**方法列表:**  

| 方法名 | 说明 | 返回值 | 参数 |
| ----- | --- | ------ | ------ |
| **rellaf_type** | 返回`no`类型 | ModelType::e().no | N/A |
| **create** | 构造一个新的Model | Model* | N/A |
| **clone** | 复制当前对象 | Model* | N/A |

## Enum
**头文件:** `enum.h` 

**说明:**   
C/C++枚举（`enum`）能力很有限，只是一个数字，没有从字符串获得枚举的能力，也不能判断一个枚举是否存在。
`Rellaf`实现了灵活的枚举类，使用同样非常简单。与`Object`类似，配合相关的宏使用。

```C++
class DemoEnum : public IEnum {
rellaf_enum_dcl(DemoEnum);

//  按照 code(int)，name(std::string) 定义，都保证唯一。
rellaf_enum_item_def(0, A);
rellaf_enum_item_def(1, B);
};
rellaf_enum_def(DemoEnum);
```

**相关宏列表:**   

| 宏名 | 说明 | 参数 |
| ----- | --- | ------ | 
| rellaf_enum_dcl | 申明`Enum`对象 | 自定义类名 | 
| rellaf_enum_def | 定义`Enum`对象 | 自定义类名 | 
| rellaf_enum_item_def | 定义枚举成员 | 自定义类名 | 
| rellaf_enum_item_code_def | 定义枚举成员(带静态数字编码) | 自定义类名 | 
| rellaf_enum | 工具宏, 等价于 Enum::e() | 自定义类名 | 
  
`Enum`的取值都是`Enum::e()`这种单例访问的模式，无法使用在switch case语句中，而这需要静态数字。为了支持有了`rellaf_enum_item_code_def`这个宏，区别与 `rellaf_enum_item_def`，会额外定义数字编码静态成员。如上面的`DemoEnum`对象，如果换成`rellaf_enum_item_code_def(0, A)`，则会有`constexpr static int DemoEnum::A_code=0`成员。

如果是C++17以上，这样就已经足够了。但是由于C++11，14，对于静态成员，一定要在类外再定义一次，然后才能使用`静态数字编码`如下：
```C++
constexpr int DemoEnum::A_code;
```

枚举成员类型是 `struct EnumItem`，**成员列表:**

| 成员名 | 说明 | 类型 |
| ----- | ---- | ------ |
| **code** | 编码 | int |
| **name** | 名字 | std::string |

**EnumItem方法列表:**

| 方法名 | 说明 | 返回值 | 参数 |
| ----- | ---- | ------ | ------ |
| **available** | 是否有效 | bool | N/A |
| **operator==** | 判断等，根据`code` | bool | EnumItem |
| **operator!=** | 判断不等，根据`code` | bool | EnumItem |
| **operator<** | 判断大小，根据`code` | bool | EnumItem |

统一使用 Enum::e() 或 rellaf_enum(Enum) 单例访问枚举类方法。

**Enum方法列表:**  

| 方法名 | 说明 | 返回值 | 参数 |
| ----- | ---- | ------ | ------ |
| **names** | 枚举名字合集 | std::map<std::string, int>& | N/A |
| **codes** | 枚举编码合集 | std::map<int, std::string>& | N/A | 
| **get** | 获得枚举值 | EnumItem, 如果不存在EnumItem.available() == false | 有两个重载: int, 根据code查找; std::string, 根据name查找 | 
| **exist** | 是否存在枚举值 | bool | 有两个重载: int, 根据code查找; std::string, 根据name查找 | 
  
  
**例子:**    
使用上文定义的`DemoEnum`
```C++
// 枚举都是单例类，通过单例方法或者 rellaf_enum宏 访问
std::string name = DemoEnum::e().A.name;                    // 返回 "A"
int code = rellaf_enum(DemoEnum).B.code;                    // 返回 1

// 静态数字编码，可用于switch case (仅C++17以上)
code = DemoEnum::A_code;                                    // 返回 1
code = DemoEnum::B_code;                                    // 返回 2
code = DemoEnum::C_code;                                    // 返回 3

// 比较
if (DemoEnum::e().B != DemoEnum::e().C) {
    // B not equal C
}

// 判断是否存在
DemoEnum::e().exist(2);                                     // 返回 true
DemoEnum::e().exist("D");                                   // 返回 false

// 取值
EnumItem name = DemoEnum::e().get("B");
if (name.available()) {
    // exist, do something
}
EnumItem code  = DemoEnum::e().get(1);
if (code.available()) {
    // exist, do something
}

// 获得取值范围
const std::map<std::string, int>& names = DemoEnum::e().names();
const std::map<int, std::string>& codes = DemoEnum::e().codes();
```


## 扩展部分

### Json

**头文件:** `json_to_model.h`

显然，这是一个提供`Model`对象和`Json`互相转换的库。当前版本使用依赖Jsoncpp实现(rapidjson性能更佳, 待调研)。

目前包含2个API：  
bool **model_to_json**(const Model* model, std::string& json_str, bool is_format = false);  
bool **json_to_model**(const std::string& json_str, Model* model);

根据字面意思，就是字符串表示的Json与`Model`类型的相互转换，`is_format`表示是否换行缩进。注意几点：
- `model_to_jso`总是返回true，`Object`或者`List`成员如果是nullptr，则输出Json的null value。
- `json_to_model`如果输入字符串parse json失败，返回false，否则返回true。
- `Model`定义的结构可能与输入的Json不一样，结构不一致的部分会跳过转换。
- Json object为null value的成员，不会进行转换。

**类型对应:**
| rellaf类型 | Jsoncpp类型 |
| ------ | -------- |
| CHAR | Json::Int |
| INT16 | Json::Int |
| INT  | Json::Int |
| INT64 | Json::Int64 |
| UINT16 | Json::UInt |
| UINT32 | Json::UInt |
| UINT64 | Json::UInt64 |
| BOOL  | Json::booleanValue |
| FLOAT  | Json::realValue |
| DOUBLE  | Json::realValue |
| STR  | Json::stringValue |

### Sql
TODO。。。

### Brpc

**头文件:** `brpc_dispatcher.h`

**主要功能:**  
- 相同API的POST和GET类型请求可以直接映射到不同的处理函数。
- 建立了HTTP API，proto接口签名，具体处理函数的直接关联。一行代码定义关联。
- 包装了HTTP请求相关上下文数据，直接传递给处理函数。
- HTTP body，request query，path variable都能自动到Model的转换。处理函数参数可指定。
- 处理函数直接返回`Model`，可自动转换成Json或字符串。

**HttpContext:**  
HTTP请求上下文数据包，原型：
```C++
struct HttpContext {
    const HttpHeader& request_header;
    const butil::IOBuf& request_body;
    HttpHeader& response_header;
    butil::IOBuf& response_body;
    std::map<std::string, std::string>& path_vars;
};
```

我们先通过一个例子来看一下最简单的用法，假设场景：  
我们需要实现rest风格接口`/api/hi/{id}`，并且能够将HTTP body转换为`Body`（`Object`），请求查询参数转换为`Params`（`Object`），路径变量转换为`Vars`(`Model`)（`{id}`就是一个路径变量，当请求路径是`/api/hi/111`时，`{id}`为`111`）。请求处理完成后，返回HTTP body字符串为“OK”拼接`{id}`的值。

1. 定义protobuf文件，生成protobuf service接口，这个过程看[brpc文档](https://github.com/brpc/brpc/blob/master/docs/cn/http_service.md)。假设我们定义的Protobuf Service是：
```protobuf
option cc_generic_services = true;
message DemoRequest {};
message DemoResponse {};
// DemoRequest，DemoResponse是protobuf要求的请求和应答类型，这个在这里不起作用，但是要写
service DemoService {
    rpc hi (DemoRequest) returns (DemoResponse);
}
```

2. 定义一个`rellaf service`
```C++
class DemoServiceImpl : public BrpcService, public DemoService {
rellaf_brpc_http_dcl(DemoServiceImpl, DemoRequest, DemoResponse);
// 定义API和处理函数的映射
rellaf_brpc_http_def_post(hi, "/api/hi/{id}", hi_handler, Plain<std::string>, Params, Vars, Body);
};
rellaf_brpc_http_def(DemoServiceImpl);
```

3. 定义接口处理函数
```C++
Plain<std::string> DemoServiceImpl::hi_handler(HttpContext& context, const Params& params, const Vars& vars, const Body& body) {
    ...
    // context.aaa
    // body.xxx
    // params.ooo
    ...
    return Plain<std::string>("OK" + vars.id());
}
```
也可以不用第3步单独定义，第2部时"一气呵成"：
```C++
...
rellaf_brpc_http_def_post(hi, "/api/hi/{id}", hi_handler, Plain<std::string>, Params, Vars, Body) {
    ...
    // 约定 Context, Params, Vars, Body 实参分别为 ctx, p, v, b
    // ctx.aaa
    // b.xxx
    // p.ooo
    ...
    return Plain<std::string>("OK" + v.id()); 
}
...
```

这就完成了一个POST接口的请求处理。GET方法类似，区别在于没有请求Body。还提供其他宏可供不同的请求类型组合。

**相关宏列表:**   

| 宏名 | 对应接口签名（说明） |
| ----- | ----------------- | 
| rellaf_brpc_http_dcl | 申明`Enum`对象，参数：自定义类名，pb request，pb response  | 
| rellaf_brpc_http_def | 定义`Enum`对象，参数：自定义类名 | 
| rellaf_brpc_http_def_get | _Ret_ _func_(HttpContext& ctx, const _Params_& p, const _Vars_& v) |  | 
| rellaf_brpc_http_def_get_param | _Ret_ _func_(HttpContext& ctx, const _Params_& p) | | 
| rellaf_brpc_http_def_get_pathvar | _Ret_ _func_(HttpContext& ctx, const _Vars_& v) | | 
| rellaf_brpc_http_def_post | _Ret_ _func_(HttpContext& ctx, const _Params_& p, const _Vars_& v, const _Body_& b) | | 
| rellaf_brpc_http_def_post_body | _Ret_ _func_(HttpContext& ctx, const _Body_& b) | | 
| rellaf_brpc_http_def_post_param | _Ret_ _func_(HttpContext& ctx, const _Params_& p) | | 
| rellaf_brpc_http_def_post_pathvar | _Ret_ _func_(HttpContext& ctx, const _Vars_& v) | | 
| rellaf_brpc_http_def_post_param_body | _Ret_ _func_(HttpContext& ctx, const _Params_& p, const _Body_& b) | | 
| rellaf_brpc_http_def_post_pathvar_body | _Ret_ _func_(HttpContext& ctx, const _Vars_& v, const _Body_& b) | | 
| rellaf_brpc_http_def_post_param_pathvar | _Ret_ _func_(HttpContext& ctx, const _Params_& p, const _Vars_& v) | | 

TODO 详细解释路劲变量和查询字符串转换。

更多Method支持，还有更多HTTP语义和特性的支持看需求逐步支持，欢迎提ISSUE。
# MiniSQL总体设计报告

## 1.MiniSQL系统概述

### 1.1 背景

#### 1.1.1 编写目的

1. 设计并实现一个精简型单用户SQL引擎MiniSQL，允许用户通过字符界面输入SQL语句实现基本的增删改查操作，并能够通过索引来优化性能。
2. 通过对MiniSQL的设计与实现，提高学生的系统编程能力，加深对数据库管理系统底层设计的理解。

#### 1.1.2 项目背景

**(TODO)**

### 1.2 功能描述

以下为本*MiniSQL*所支持的操作与指令,其中与主流数据库( *MySQL*等 )同名的指令,功能和语法都与其相同,不再赘述.

* database

  * create database
  * drop database
  * use database
  * show databases

* table
  * create table
    * 支持的数据类型: *int, float, char(n)*
    * 支持的约束: *primary key, unique, not null*
  * drop table
  * show tables

* index
  * create index
    * 支持的索引类型: *B+树*
  * drop index
  * show indexes

* 表操作

  * select
  * insert
  * delete
  * update

* sql脚本
  * execfile:支持从文件中读取sql语句并执行
    * execfile "a.txt";

### 1.3 运行环境和配置

主要在*Windows*下的*WSL*中运行, 发行版为*Ubuntu 22.04*

* *gcc & g++* version: 11.3.0
* *cmake* version 3.22.1
* *GNU gdb* version 12.1

### 1.4 参考资料

1. https://www.yuque.com/yingchengjun/minisql
2. https://git.zju.edu.cn/zjucsdb/minisql

## 2. MiniSQL系统结构设计

### 2.1 总体设计

软件系统架构图如下:\
![架构](./figure/架构.png)

* 在系统架构中，解释器SQL Parser在解析SQL语句后将生成的语法树交由执行器Executor处理。执行器则根据语法树的内容对相应的数据库实例（DB Storage Engine Instance）进行操作。

* 每个DB Storage Engine Instance对应了一个数据库实例（即通过CREATE DATABSAE创建的数据库）。在每个数据库实例中，用户可以定义若干表和索引，表和索引的信息通过Catalog Manager、Index Manager和Record Manager进行维护。目前系统架构中已经支持使用多个数据库实例，不同的数据库实例可以通过USE语句切换（即类似于MySQL的切换数据库）.

### 2.2 DISK AND BUFFER POOL MANAGER

#### 2.2.1 位图页 Bitmap Page

位图页是Disk Manager模块中的一部分，是实现磁盘页分配与回收工作的必要功能组件。位图页与数据页一样，占用`PAGE_SIZE`（4KB）的空间，标记一段连续页的分配情况。

如下图所示，位图页由两部分组成，一部分是用于加速Bitmap内部查找的元信息（Bitmap Page Meta），它包含当前已经分配的页的数量（`page_allocated_`）以及下一个空闲的数据页(`next_free_page_`)。除去元信息外，页中剩余的部分就是Bitmap存储的具体数据，其大小`BITMAP_CONTENT_SIZE`可以通过`PAGE_SIZE - BITMAP_PAGE_META_SIZE`来计算。剩余部分中每一个bit都可以记录一个对应的页的分配情况。自然而然，这个Bitmap Page能够支持最多纪录`BITMAP_CONTENT_SIZE * 8`个连续页的分配情况。

![bitmap](./figure/bitmap.png)

#### 2.2.2 磁盘数据页管理

如下图所示，我们将一个位图页加一段连续的数据页看成数据库文件中的一个分区（Extent），再通过一个额外的元信息页来记录这些分区的信息，使得磁盘文件能够维护更多的数据页信息。

![disk_manager](./figure/disk_manager.png)

Disk Meta Page是数据库文件中的第0个数据页，它维护了分区相关的信息，包括已分配的Page数量（`num_allocated_pages_`）、已分配的分区数（`num_extents_`）、每个分区已分配的Page数量（`extent_used_page_`）。

为了便于后续设计，我们将页面的页号分为物理页号和逻辑页号。上层建筑中的页号都是逻辑页号，而磁盘文件中的页号都是物理页号。只有数据页拥有逻辑页号。假如每个分区能够支持3个数据页，则物理页号与逻辑页号的对应关系如下表所示：

| 物理页号 | 0 | 1 | 2 | 3 | 4 | 5 | 6 | ... |
| ------ | - | - | - | - | - | - | - | --- |
| 职责    | 磁盘元数据 | 位图页 | 数据页 | 数据页 | 数据页 | 位图页 | 数据页 | ... |
| 逻辑页号 | | | 0 | 1 | 2 | | 3 | ... |

#### 2.2.3 缓冲池管理

为了实现缓冲池管理，我们首先需要实现一个替换策略。在本次实验中，我们选择了LRU替换策略，以在缓冲池没有空闲页时决定替换哪一个数据页。LRU，即Least Recently Used，最近最少使用。在LRU替换策略中，我们维护一个链表，链表中的每一个节点都是一个数据页，链表的头部是最近使用的数据页，链表的尾部是最久未使用的数据页。当缓冲池中没有空闲页时，我们将链表尾部的数据页替换出去，将新的数据页插入到链表头部。

Buffer Pool Manager负责从Disk Manager中获取数据页并储存到内存中，并在必要时将脏页面转储到磁盘中（如需要为新的页面腾出空间）。在内存中，所有的页面都由`Page`对象表示，每个`Page`对象都包含了一段连续的内存空间`data_`和与该页相关的信息，包括逻辑页号（`page_id_`）、Pin Count（`pin_count_`）、脏页标记（`is_dirty_`）。如果固定（Pin）了一个页面，则该页面的Pin Count加一，如果解固定（Unpin）了一个页面，则该页面的Pin Count减一。如果一个页面的Pin Count为0，则该页面可以被替换出去。而决定一个页面是否需要被替换出去，则需要用到上述部分的LRU Replacer。

### 2.3 RECORD MANAGER

#### 2.3.1 RowID

该class用来记录一条record的物理存储位置,即记录所在的页号和记录在该页中的序号.

* 数据结构
  * 成员变量

    ```cpp
    page_id_t page_id_: 记录该条记录存储的页号,默认为*INVALID_PAGE_ID*
    uint32_t slot_num_: 记录该条record在该页中的序号
    ```

  * 运算符重载
    * 重载"=="来判断是否相同
  
  * 成员函数

    ```cpp
    1. 构造函数
    2. inline page_id_t GetPageId();
        返回 page_id_
    3. inline uint32_t GetSlotNum();
        返回 slot_num_
    4. inline void Set(page_id_t page_id, uint32_t slot_num);
        设置实例的成员
    ```

#### 2.3.2 Field

该class用来记录一条record中的一个field,即一个属性.

* 数据结构
  * 成员变量

    ```cpp
    union Val {
    int32_t integer_;
    float float_;
    char *chars_;
    } value_;       //用来存储该属性的值
    TypeId type_id_;//枚举类,记录该属性的数据类型
                    //用来选择上面的value_中的哪一个
    uint32_t len_;  //记录该属性数据的占用空间
    bool is_null_{false};//记录该属性是否为NULL
    bool manage_data_{false};//表示是否需要自己删除该属性
    ```

#### 2.3.2 ROW

* 数据结构
  * 成员变量
  
    ```cpp
    RowId rid_;//记录该条record的RowId,具体可见2.3.1
    std::vector<Field *> fields_//记录该条record的所有属性及值
    ```

  * 成员函数

    ```cpp


### 2.4 INDEX MANAGER

#### 2.4.1 B+树数据页

B+树中每个节点都对应一个数据页。我们首先实现了一个内部节点和叶子节点所需要的公共父类`BPlusTreePage`，它包括了中间节点`BPlusTreeInternalPage`与叶子节点`BPlusTreeLeafPage`的共同部分，包括：

* `page_type_`: 标记数据页是中间结点还是叶子结点；
* `key_size_`: 当前索引键的长度，
* `lsn_`: 数据页的日志序列号，目前不会用到，如果之后感兴趣做Crash Recovery相关的内容需要用到；
* `size_`: 当前结点中存储Key-Value键值对的数量；
* `max_size_`: 当前结点最多能够容纳Key-Value键值对的数量；
* `parent_page_id_`: 父结点对应数据页的page_id;
* `page_id_`: 当前结点对应数据页的page_id。

对于中间节点，由于不储存数据，只需要按照顺序存储$m$个键和$m+1$个指针（这些指针记录的是子结点的`page_id`）。由于键和指针的数量不相等，因此我们需要将第一个键设置为INVALID，也就是说，顺序查找时需要从第二个键开始查找。在任何时候，每个中间结点至少是半满的（Half Full）。当删除操作导致某个结点不满足半满的条件，需要通过合并（Merge）相邻两个结点或是从另一个结点中借用（移动）一个元素到该结点中（Redistribute）来使该结点满足半满的条件。当插入操作导致某个结点溢出时，需要将这个结点分裂成为两个结点。在B+树中，键的类型为`GenericKey`，它由`KeyManager`对`Row`类型的键进行序列化而得到，并可以通过`KeyManager`对两个`GenericKey`进行比较。

对于叶子节点，其存储实际的数据，按照顺序存储$m$个键和$m$个值。其值为`RowId`。叶结点和中间结点一样遵循着键值对数量的约束，同样也需要完成对应的合并、借用和分裂操作。

#### 2.4.2 B+树索引

我们所设计的B+树只支持`Unique Key`。上述完成的B+树数据页需要通过`BPlusTree`类来进行管理。`BPlusTree`类中包括了插入、删除、合并、借用和分裂等操作。提供给外部的接口如下：

* `bool IsEmpty()`: 判断B+树是否为空；
* `bool Insert(GenericKey *key, const RowId &value, Transaction *transaction = nullptr)`：插入键值对，如果键已经存在，则返回false，否则返回true；
* `void Remove(const GenericKey *key, Transaction *transaction = nullptr)`：删除键值对；
* `bool GetValue(const GenericKey *key, std::vector<RowId> &result, Transaction *transaction = nullptr)`：根据键查找对应的值，并将结果存储在`result`中。如果键不存在，则返回false，否则返回true；
* `IndexIterator Begin(const GenericKey *key);`：返回第一个键值对的迭代器；
* `IndexIterator End();`：返回最后一个键值对的迭代器；
* `void Destroy(page_id_t current_page_id = INVALID_PAGE_ID);`：销毁B+树，如果`current_page_id`为`INVALID_PAGE_ID`，则销毁整棵树，否则销毁以`current_page_id`为根的子树。

#### 2.4.3 B+树索引迭代器

B+树迭代器`IndexIterator`用来遍历B+树中的键值对。主要的成员变量为当前的page_id（`current_page_id`）与在该叶子节点页面中的索引值（`item_index`）。主要通过重载以下几个操作符来实现迭代器的功能：

* `std::pair<GenericKey *, RowId> operator*()`：返回当前键值对；
* `IndexIterator &operator++()`：将迭代器指向下一个键值对；
* `bool operator==(const IndexIterator &itr) const`：判断两个迭代器是否相等；
* `bool operator!=(const IndexIterator &itr) const`：判断两个迭代器是否不相等。

### 2.5 CATALOG MANAGER

### 2.6 PLANNER AND EXECUTOR

#### 2.6.1 Parser生成语法树

在本实验提供的代码中已经设计好了Parser模块，其生成的语法树数据结构如下：

```cpp
/**
 * Syntax node definition used in abstract syntax tree.
 */
struct SyntaxNode {
  int id_;    /** node id for allocated syntax node, used for debug */
  SyntaxNodeType type_; /** syntax node type */
  int line_no_; /** line number of this syntax node appears in sql */
  int col_no_;  /** column number of this syntax node appears in sql */
  struct SyntaxNode *child_;  /** children of this syntax node */
  struct SyntaxNode *next_;   /** siblings of this syntax node, linked by a single linked list */
  char *val_; /** attribute value of this syntax node, use deep copy */
};
typedef struct SyntaxNode *pSyntaxNode;
```

例如，对于`select * from t1 where id = 1 and name = "str";`这一条SQL语句，生成的语法树如下图所示：

![syntax_tree](./figure/syntax_tree.png)

#### 2.6.2 Planner生成查询计划

Planner部分需要先遍历语法树，并调用Catalog Manager检查语法树中的信息是否正确，如表、列是否存在，谓词的值类型是否与column类型对应等等，随后将这些词语抽象成相应的表达式。解析完成后，Planner根据改写语法树后生成的可以理解的Statement结构，生成对应的Plannode，并将Planndoe交由executor进行执行。

#### 2.6.3 Executor执行查询计划

在本次实验中，我们采用的算子的执行模型为火山模型。执行引擎会将整个 SQL 构建成一个 Operator 树，查询树自顶向下的调用接口，数据则自底向上的被拉取处理。每一种操作会抽象为一个 Operator，每个算子都有 `Init()` 和 `Next()` 两个方法。`Init()` 对算子进行初始化工作。`Next()` 则是向下层算子请求下一条数据。当 `Next()` 返回 false 时，则代表下层算子已经没有剩余数据，迭代结束。

本次实验我们实现了select、index select、insert、update、delete五个算子。 对于每个算子，都实现了 Init 和 Next 方法。 Init 方法初始化运算符的内部状态，Next 方法提供迭代器接口，并在每次调用时返回一个元组和相应的 RID。对于每个算子，我们假设它在单线程上下文中运行，并不需要考虑多线程的情况。每个算子都可以通过访问 ExecuteContext来实现表的修改，例如插入、更新和删除。 为了使表索引与底层表保持一致，插入删除时还需要更新索引。

* SeqScanExecutor 对表执行一次顺序扫描，一次Next()方法返回一个符合谓词条件的行。顺序扫描的表名和谓词（Predicate）由SeqScanPlanNode 指定。
* IndexScanExecutor 对表执行一次带索引的扫描，一次Next()方法返回一个符合谓词条件的行。为简单起见，IndexScan仅支持单列索引。
* InsertExecutor 将行插入表中并更新索引。要插入的值通过ValueExecutor生成对应的行，随后被拉取到InsertExecutor中。
* ValueExecutor主要用于insert into t1 values(1, "aaa", null, 2.33);语句。插入的值以vector形式存储在ValuesPlanNode中，ValueExecutor调用Next()方法一次返回一个新的行。
* UpdateExecutor 修改指定表中的现有行并更新其索引。UpdatePlanNode 将有一个 SeqScanPlanNode 作为其子节点，要更新的值通过SeqScanExecutor提供。
* DeleteExecutor 删除表中符合条件的行。和Update一样，DeletePlanNode 将有一个 SeqScanPlanNode 作为其子节点，要删除的值通过SeqScanExecutor提供。

除了以上几个算子，我们还需要实现创建删除查询数据库、数据表、索引，执行文件中的SQL语句等函数。由于这些操作对应的语法树较为简单，因此不用通过Planner生成查询计划。

Executor模块的具体执行过程隐藏为private成员函数，外部只需要调用`dberr_t Execute(pSyntaxNode ast);`接口，传入语法树根节点，来执行对应的操作。

## 3.测试方案和测试样例

### 3.1 函数测试

首先是采用*MiniSQL*提供的*test*文件夹中的测试样例进行测试,主要用来测试该数据库内部各个函数是否运行正确,测试结果如下:

![普通test](./figure/普通test.png)

其中主要分为以下几个模块:

* *BufferPoolManagerTest*
  * BinaryDataTest
    主要用来测试*BufferPoolManager*的基本功能:
    * 新建页*NewPage*,并在达到最大页数时返回*nullptr*
    * 修改页中数据并可再次读取
    * 在*UnpinPage*之后,可以用*NewPage*将该页替换掉

* *LRUReplacerTest*
  * SampleTest
    * 与上文(*BufferPoolManager*)中测试相似,只是此处采用了*LRUReplacer*作为*BufferPoolManager*的替换策略,也就是说,当*BufferPoolManager*中的页数达到最大时,会将最近最少使用的页替换掉.
* *DiskManagerTest*
  作为*BufferPoolManager*真正接触磁盘的中间类,用来实现真正的磁盘读写操作,主要测试了以下几个功能:
  * BitMapPageTest
  * FreePageAllocationTest
* *TupleTest*
  * FieldSerializeDeserializeTest
    主要用来测试*Field*类的序列化和反序列化
  * RowTest
    主要用来测试Tuple(Row)的插入与删除,主要是通过*TablePage*这个类作为调用端,调用Row中的序列化和反序列化函数,将Row插入到*TablePage*中;然后再将Row中的数据反序列化出来,与原来的数据进行比较,看是否相同;最后再从*TablePage*中删除Row,
* *TableHeapTest*
  * TableHeapSampleTest
    主要是用来测试*TableHeap*该类的插入与获取Tuple的功能. 与上文中(*TupleTest::RowTest*)仅仅测试一条数据不同,这里测试了较大数据规模(10000条)的插入与查询,并且在插入时,覆盖了三种所支持的数据类型(*int,float,char*).
* *BPlusTreeTests*
  * IndexGenericKeyTest
  * IndexSimpleTest
  * SampleTest
    向B+树中插入数据,并且在初始化数据之后采用shuffle打乱来模拟索引后的顺序,最后再进行查询,看是否能够正确的查询到数据.
  * IndexIteratorTest
* *PageTests*
  * IndexRootsPageTest
    主要是用来测试*IndexRootsPage*类的插入、获取、删除功能.并且查看获取的数据的正确性.
* *CatalogTest*
  * CatalogMetaTest
    主要测试*CatalogMeta*类的序列化和反序列化功能
  * CatalogTableTest
  * CatalogIndexTest
    上面两个测试方法相似,只是测试的对象不同,一个是*Table*,一个是*Index*.

    其中心思想都是先创建一个数据库实例,并对该实例中的catalogMeta进行操作(createTable和CreateIndex),然后再将该实例序列化到磁盘中,再从磁盘中反序列化出来,看是否与原来的相同.
* *ExecutorTest*
  以下的4个测试都是在setup初始化之后的这个数据库中进行操作的,在初始化中,向表中插入了1000条数据,表定义如下:

  ```sql
  create table table-1(id int,
                       name char(64),
                       account float);
  ```

  * SeqScanTest
    测试*select*功能:SELECT id FROM table-1 WHERE id < 500;
  * DeleteTest
    测试*delete*功能:DELETE FROM table-1 WHERE id == 50;
  * RawInsertTest
    测试*insert*功能:INSERT INTO table-1 VALUES (1001, "aaa", 2.33);
  * UpdateTest
    测试*update*功能:UPDATE table-1 SET name = "minisql" where id = 500;

### 3.2 数据库测试

该模块则是对最后生成的数据库系统进行测试,主要是测试数据库的基本功能,包括*select,insert,delete,update*等功能.

由于涉及语句较多,由此此处主要采用*sql*文件的形式进行测试,并使用*execfile*指令调用

* table的create/drop功能

  ```sql
  create table person ( 
    height float,
    pid int,
    name char(32),
    identity char(128),
    age int unique,
    primary key(pid)
  );

  insert into person values (1300.0, 0, "Tom", "cf0a2386-2435-423f-86b8-814318bedbc7", 0);

  drop table person;
  ```

  结果如下,成功运行:
  ![table_create_drop](./figure/table_create_drop.png)
* table的insert/delete功能
  此处有两组不同的delete语句,测试中是分别测试的:
  
  ```sql
  create table person ( 
    height float,
    pid int,
    name char(32),
    identity char(128) unique,
    age int,
    primary key(pid)
  );
  <!-- 插入20条数据 -->

  <!-- 第一组删除语句 -->
  delete from person where pid = 15;
  delete from person where height = 173.5;
  delete from person where age = 20 and height > 175.5;
  delete from person where height = 175.1;
  delete from person where name = "Person20";
  delete from person where identity = "000017";
  delete from person where identity = "000016" and age = 29;

  <!-- 第二组删除语句 -->
  delete from person where pid >= 15;
  delete from person where height < 173.5;
  delete from person where age <= 20 and height < 170 and identity = "000016";
  delete from person where age <= 20 and height >= 175;
  delete from person where age <= 20 and height < 170 and name = "Person16";

  <!-- 最后查询与drop相同 -->
  select * from person;

  drop table person;
  ```

  结果如下:
  * 第一组删除语句:
  ![table_insert_delete0](./figure/delete0.png)
  * 第二组删除语句:
  ![table_insert_delete1](./figure/delete1.png)

* table的select功能
  与上文一样,也有两组不同的查询语句:

  ```sql
  create table person ( 
    height float,
    pid int,
    name char(32),
    identity char(128) unique,
    age int,
    primary key(pid)
  );
  <!-- 插入20条数据 -->

  <!-- 第一组查询语句 -->
  select * from person where pid = 15;
  select * from person where height = 173.5;
  select * from person where age = 20 and height > 175.5;
  select * from person where height = 175.1;

  <!-- 第二组查询语句 -->
  select * from person where pid >= 15;
  select * from person where height < 173.5;
  select * from person where age <= 20 and height < 170;

  <!-- 最后都drop掉 -->
  drop table person;
  ```
  
  结果如下:
  * 第一组查询语句:
  ![table_select1](./figure/select0.png)
  * 第二组查询语句:
  ![table_select2](./figure/select1.png)
* index的create/drop功能

  ```sql
  create table person ( 
    height float unique,
    pid int,
    name char(32),
    identity char(128) unique,
    age int unique,
    primary key(pid)
  );
  <!-- 插入一定量的数据(此处20条),为节省空间省略 -->
  create index idx_height on person(height);
  create index idx_identity on person(identity);
  create index idx_age on person(age);

  select * from person where age > 24;
  select * from person where identity = "Person15";
  select * from person where height <= 176.3;

  drop index idx_height;
  drop index idx_identity;
  drop index idx_age;

  drop table person;
  ```

  结果如下:
  ![index_create_drop](./figure/index.png)
* 压力测试
  分别向表中插入1000,1w,10w条数据,并在插入之后create index.

## 4.分组与设计分工

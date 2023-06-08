# MiniSQL总体设计报告

## 1.MiniSQL系统概述

### 1.1 背景

#### 1.1.1 编写目的

1. 设计并实现一个精简型单用户SQL引擎MiniSQL，允许用户通过字符界面输入SQL语句实现基本的增删改查操作，并能够通过索引来优化性能。
2. 通过对MiniSQL的设计与实现，提高学生的系统编程能力，加深对数据库管理系统底层设计的理解。

#### 1.1.2 项目背景

的

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
  * show indexex

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

### 2.3 RECORD MANAGER

### 2.4 INDEX MANAGER

### 2.5 CATALOG MANAGER

### 2.6 PLANNER AND EXECUTOR

## 3.测试方案和测试样例

## 4.分组与设计分工

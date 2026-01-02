# 五子棋 (Gomoku) C++ 项目

## 简介
这是一个带有命令行界面的五子棋 C++ 实现。主要特性包括：
- **面向对象设计**：采用模块化架构，包含 `GameEngine`（游戏引擎）、`RuleSet`（规则集）、`Board`（棋盘）、`Player`（玩家）和 `Renderer`（渲染器）。
- **五子棋规则**：
  - 黑方先行（天元开局）。
  - 黑方禁手：长连（6子及以上）、三三、四四。
  - "五连优先"：如果一步棋同时形成五连和禁手，判五连获胜。
  - 白方长连获胜。
  - "立即声明"：白方必须立即指出黑方的禁手。
- **游戏模式**：人对人、人对AI、AI对人。
- **计时系统**：每步限时 15 秒，超时会有警告（最多 3 次）。

## 构建说明

### 前置要求
- 支持 C++17 的编译器 (GCC, Clang, MSVC)。
- CMake (可选，但推荐)。
- Windows 操作系统（以获得更好的渲染效果）。

### 使用 CMake (Windows MinGW)
```bash
mkdir build
cd build
cmake -G "MinGW Makefiles" ..
cmake --build .
.\Gomoku.exe
```

### 手动编译
```bash
g++ -std=c++17 -I./include src/*.cpp main.cpp -o gomoku
./gomoku
```

## 玩法说明
- **输入**：输入坐标，例如 `H8`（列号字母 + 行号数字）。
- **命令**：
  - `q` 或 `quit`：认输。
  - `claim`：举手声明禁手（仅在提示时可用）。
  - `draw`：提议和棋。
  - `undo`：悔棋。
  - `save`：保存对局。

## 架构
- **GameEngine**：管理游戏循环、计时和玩家回合。
- **RuleSet**：游戏规则的抽象基类。`GomokuRuleSet` 实现了具体逻辑。
- **Board**：管理网格状态。
- **Player**：抽象基类。`HumanPlayer` 处理输入，`AIPlayer` 使用启发式算法，对于五子棋游戏来说，它足够智能且快速，无需在每步仅15秒的限制下使用一个AI模型。
- **Renderer**：处理控制台输出。

## 禁手逻辑
在 `GomokuRuleSet` 中使用模式匹配实现。
- **长连 (Overline)**：检查是否超过 5 子。
- **三三 (Three-Three)**：检查是否存在 >=2 个“活三”（如 `01110` 的模式）。
- **四四 (Four-Four)**：检查是否存在 >=2 个“四”（可以变成五连的模式）。
详情请参阅 docs 目录。

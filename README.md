# Hydrogen Godot Behaviors

- [x] TODO: Blackboard
- [ ] TODO: Pipelines - IN PROGRESS
  - [ ] TODO: Node Binding Code generator
  - [ ] TODO: Graph Nodes - IN PROGRESS
- [ ] TODO: Behavior Trees - IN PROGRESS
- [ ] TODO: Hierarchical FSMs
- [ ] TODO: Agents
- [ ] TODO: Steering
  - [ ] TODO: Navigation-integration
- [ ] TODO: Generator Pipelines
- [ ] TODO: Scheduling Pipelines

## Behavior Tree Nodes

- [x] Base Node
- [ ] Task Nodes
  - [ ] Count Down Node (TEST)
  - [ ] Perform Interruption Node (TEST)
  - [ ] Print Message Node (TEST)
  - [ ] Set Bool Node
  - [x] Wait Nodes
    - [x] Wait For Seconds Node
    - [x] Wait For Milliseconds Node
    - [x] Wait For Ticks Node
    - [x] Wait For Realtime Seconds Node
- [x] Decorator Node
  - [ ] Interrupter Node (TEST)
  - [ ] Inverter Node (TEST)
  - [ ] Limit Node (TEST)
  - [ ] Until Fail Node (TEST)
- [ ] Parallel Node
- [ ] Composite Node (NEEDS TEST)
- [x] Sequence Node
- [ ] Selector Node (NEEDS TEST)

## Library Compatibility

Currently this is built against the `godot-4.6.2-stable` tagged commit in the `godot-cpp`
Doctest is built against `v2.4.12` tagged commit.

Compiles:

- [x] Linux/BSD
- [x] Windows 64-bit
- [x] Mac OSX
- [ ] iOS
- [x] Android

Platforms tested against:

- [x] Linux/BSD
- [x] Windows 64-bit
- [x] Mac OSX
- [ ] iOS
- [ ] Android

## Building

- Building for Editor Play/General testing: `scons target=template_debug tests=yes`
  - To build without Unit tests, exclude the `tests=yes` arg
- Building for optimized testing: `scons target=template_release`
- For windows, I recommend using the MingW-w64 toolchain if you don't already have the MSVC toolchain setup

## Demo Project

You can open the `demo/` folder from the Godot 4.4.X editor launcher, a project file is already setup.

Alternatively, you can launch directly from the command line:

1. Navigate to the `demo/` sub-folder
2. run `PATH/TO/GODOT_EDITOR_4_6_X --editor .`

To run the project directly, ensure the `template_debug` configuration is built and use:
`PATH/TO/GODOT_EDITOR_4_6_X --project .`

# Contributions / Attributions

This project was created from the [godot-cpp-template](https://github.com/godotengine/godot-cpp-template), and it's
general instructions should still apply for the most part.

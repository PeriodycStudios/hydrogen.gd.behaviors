# Hydrogen Godot Behaviors

- [x] TODO: Blackboard
- [ ] TODO: Pipelines
  - [ ] TODO: Node Binding Code generator 
  - [ ] TODO: Graph Nodes
- [ ] TODO: Behavior Trees
- [ ] TODO: Hierarchical FSMs
- [ ] TODO: Agents
- [ ] TODO: Steering
  - [ ] TODO: Navigation-integration

## Library Compatibility
Currently this is built against the `godot-4.4.1-stable` tagged commit in the `godot-cpp`
Doctest is built against `v2.4.12` tagged commit.

Platforms tested against:
- [x] Linux/BSD
- [ ] Windows 64-bit
- [ ] Mac OSX
- [ ] iOS
- [ ] Android

## Building
- Building for Editor Play/General testing: `scons target=template_debug tests=yes`
  - To build without Unit tests, exclude the `tests=yes` arg
- Building for optimized testing: `scons target=template_release`

## Demo Project

You can open the `demo/` folder from the Godot 4.4.X editor launcher, a project file is already setup.

Alternatively, you can launch directly from the command line:
1. Navigate to the `demo/` sub-folder
2. run `PATH/TO/GODOT_EDITOR_4_4_X --editor .`

# Contributions / Attributions

This project was created from the [godot-cpp-template](https://github.com/godotengine/godot-cpp-template), and it's
general instructions should still apply for the most part.

extends Node

@export var blackboard_a: HydrogenBlackboard = null

@export var blackboard_b: HydrogenBlackboard = null


func try_running_tests():
	if HydrogenBehaviorServer.has_method("run_tests"):
		HydrogenBehaviorServer.run_tests()


func try_exporting_type_infos():
	var dict: Dictionary = HydrogenBehaviorServer.blackboard_export_type_infos()
	print("Number of type infos: ", dict.size())
	var json_str: String = JSON.stringify(dict)
	print(json_str)


func try_blackboard_create_and_destruct():
	var bb_rid: RID = HydrogenBehaviorServer.blackboard_create()
	HydrogenBehaviorServer.blackboard_set_int(bb_rid, "MyInt", 42)
	print("My Int: ", HydrogenBehaviorServer.blackboard_get_int(bb_rid, "MyInt"))

	HydrogenBehaviorServer.free_rid(bb_rid)


func try_blackboard_create_and_destruct_by_ref():
	var bb: HydrogenBlackboard = HydrogenBlackboard.new()
	bb.set_int("MyInt", 42)

	print("My Int: ", bb.get_int("MyInt"))


func try_testing_existing_blackboards():
	if not blackboard_a or not blackboard_b:
		return

	if blackboard_a.parent == blackboard_b:
		print(blackboard_a, "'s parent is ", blackboard_b)
	else:
		if blackboard_b.parent == blackboard_a:
			print(blackboard_b, "'s parent is ", blackboard_a)
		else:
			print("neither a nor b have each other as parents")

	var entries_a: Dictionary = blackboard_a.export_entries()
	print("A's contents: \n", entries_a)

	var entries_b: Dictionary = blackboard_b.export_entries()
	print("B's contents: \n", entries_b)

	var my_color_test: Color = blackboard_a.get_color(&"favorite_color")
	if my_color_test != null:
		var my_color: Color = my_color_test
		print("My Color: ", my_color)

	var password_test: Variant = blackboard_a.try_get_entry(&"password")
	if password_test != null:
		var password: String = password_test
		print("Password: ", password)

	var bounds_test: Variant = blackboard_a.try_get_entry(&"bounds")
	if bounds_test != null:
		var bounds: AABB = bounds_test
		print("Bounds: ", bounds)
		print(blackboard_b.get_aabb(&"bounds"))

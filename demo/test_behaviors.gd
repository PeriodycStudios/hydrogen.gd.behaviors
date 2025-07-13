extends Node

func try_running_tests():
	if (BehaviorServer.has_method("run_tests")):
		BehaviorServer.run_tests()

func try_exporting_type_infos():
	var dict: Dictionary = BehaviorServer.blackboard_export_type_infos()
	print("Number of type infos: ", dict.size())
	var json_str: String = JSON.stringify(dict)
	print(json_str)

func try_blackboard_create_and_destruct():
	var bb_rid: RID = BehaviorServer.blackboard_create()
	BehaviorServer.blackboard_set_int(bb_rid, "MyInt", 42)
	print("My Int: ", BehaviorServer.blackboard_get_int(bb_rid, "MyInt"))

	BehaviorServer.free_rid(bb_rid)

func try_blackboard_create_and_destruct_by_ref():
	var bb: HydrogenBlackboard = HydrogenBlackboard.new()
	bb.set_int("MyInt", 42)

	print("My Int: ", bb.get_int("MyInt"))

extends Node

func try_running_tests():
	BehaviorServer.run_tests()

func try_exporting_type_infos():
	var dict: Dictionary = BehaviorServer.blackboard_export_type_infos()
	var json_str: String = JSON.stringify(dict)
	print(json_str)

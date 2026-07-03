# Button Lifecycle

Task Panel buttons are purely virtual. They do not exist as independent BWE Surface/Window objects. 

They are dynamically constructed `TaskBtn` structs held in the `s_task_buttons` array *during* the render pass.

## Hover & Event Handling
The `task_panel_event_callback` iterates over `s_task_buttons`. Because the array is synchronized perfectly with the render pass, mouse coordinates map 1:1 with the dynamically scaled button hitboxes.

There are no orphan buttons or memory leaks because the list is completely purged (`s_task_btn_count = 0`) at the start of every render pass.

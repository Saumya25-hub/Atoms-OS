# Call Graphs

## System Initialization Call Graph

```text
kernel_main() (Assumed from `kernel.c`)
    │
    ├──> BOSurface_Init()
    │
    ├──> BOCompositor_Initialize()
    │      ├──> BOCompositorSurface_InitPool()
    │      ├──> BOCompositorStack_Init()
    │      ├──> BOCompositorClip_Init()
    │      └──> BOCompositorDamage_Init()
    │
    ├──> BWE_Initialize()
    │      └──> BWE_EventQueue_Clear()
    │
    └──> Shell_Init() (Implied from desktop_shell.c)
           ├──> taskbar_initialize()
           │      ├──> BOS_CreatePanel(Desktop, Taskbar)
           │      └──> BOS_CreatePanel(Desktop, StartMenu)
           │
           └──> Shell_RegisterApp() (Called for all apps)
```

## Render Loop Call Graph

```text
Main_Loop()
    │
    ├──> if (BOCompositor_HasDamage())
    │      │
    │      └──> BOCompositor_ComposeFrame()
    │             ├──> BOCompositorStack_GetSortedVisible()
    │             │      └──> Insertion Sort execution
    │             │
    │             ├──> Pass 1: Occlusion Testing
    │             │
    │             ├──> Pass 2: Back-to-Front Render
    │             │      ├──> BOCompositorClip_PushRect()
    │             │      ├──> g_render_callback(surface_handle)  <-- Hook back to BWE
    │             │      │      └──> BWE_Window->on_render()
    │             │      │             ├──> Shell_DrawWallpaper() [If Desktop]
    │             │      │             ├──> taskbar_render_callback() [If Taskbar]
    │             │      │             └──> App Specific Render Calls
    │             │      └──> BOCompositorClip_PopRect()
    │             │
    │             └──> BOCompositor_ClearDamage()
```

## Event Dispatching Call Graph

```text
Interrupt Handler (e.g., PS/2 Mouse)
    │
    └──> input_abstraction -> BWE_EventQueue_Push()

Main_Loop()
    │
    └──> BOS_ProcessEvent()
           ├──> BWE_EventQueue_Pop()
           ├──> (If Mouse Event)
           │      ├──> BWE_HitTest(x, y)
           │      │      └──> Iterates Z-Order Stack backwards (Front to Back)
           │      ├──> BOS_SetFocus(hit_window)
           │      └──> hit_window->on_event()
           │             └──> Control-specific click handling (e.g. `button.on_click()`)
           │
           └──> (If Keyboard Event)
                  └──> g_focused_window_id->on_event()
```

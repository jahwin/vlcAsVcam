-- VLC Lua Extension: VCam
-- This is the official VLC-supported way to add a clickable menu item in VLC
-- (it appears under the "Extensions" menu).

local dlg = nil
local status_label = nil

function descriptor()
    return {
        title = "VCam",
        version = "0.1",
        author = "vlcVcam",
        shortdesc = "VCam controls",
        description = "Adds an Extensions menu item: Start VCam",
        capabilities = { "menu" }
    }
end

local function start_vcam()
    vlc.msg.info("[VCam] Start VCam clicked")
    if status_label ~= nil then
        status_label:set_text("Start VCam clicked.")
    end
end

function activate()
    -- Called when the extension is activated.
    vlc.msg.info("[VCam] extension activated")

    -- On macOS, VLC often shows only the extension name in the menu (no submenu).
    -- So we always provide a small dialog with an explicit "Start VCam" button.
    dlg = vlc.dialog("VCam")
    status_label = dlg:add_label("Ready.", 1, 1, 2, 1)
    dlg:add_button("Start VCam", start_vcam, 1, 2, 1, 1)
    dlg:add_button("Close", function()
        -- Important: if we only close the dialog, VLC still considers the extension
        -- "active", and clicking Extensions → VCam again will NOT call activate().
        -- So we explicitly deactivate the extension to allow reopening.
        if vlc ~= nil and vlc.deactivate ~= nil then
            vlc.deactivate()
        else
            deactivate()
        end
    end, 2, 2, 1, 1)
end

function deactivate()
    vlc.msg.info("[VCam] extension deactivated")
    if dlg ~= nil then
        dlg:delete()
        dlg = nil
        status_label = nil
    end
end

function close()
    deactivate()
end

function menu()
    -- If VLC displays submenu items on your platform, you'll see:
    -- VLC → Extensions → VCam → Start VCam
    return { "Start VCam" }
end

function trigger_menu(id)
    if id == 1 then
        start_vcam()
    end
end



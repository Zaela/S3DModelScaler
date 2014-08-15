
local eqg = require "luaeqg"

local list = iup.list{visiblelines = 10, expand = "VERTICAL", visiblecolumns = 12}

local wld_list, wld_ent, cur_name

local ipairs = ipairs
local pairs = pairs
local pcall = pcall

local function ListModels(wld_list)
	list[1] = nil
	list.autoredraw = "NO"
	for i, name in ipairs(wld_list) do
		list[i] = name:sub(1, 3)
	end
	list.autoredraw = "YES"
end

local function FindWLD(dir)
	for i, ent in ipairs(dir) do
		if ent.name:find("%.wld$") then
			local s, err = pcall(eqg.OpenEntry, ent)
			if s then
				s, err = pcall(wld.Read, ent)
				if s then
					wld_ent = ent
					wld_list = err
					ListModels(err)
					return
				end
			end
			return error_popup(err)
		end
	end
end

function UpdateModelList(path)
	wld_list = nil
	wld_ent = nil
	cur_name = nil
	eqg.CloseDirectory(open_dir)
	open_path = path
	local s, dir = pcall(eqg.LoadDirectory, path)
	if not s then
		error_popup(dir)
		return
	end
	open_dir = dir
	FindWLD(dir)
end

function list:action(str, pos, state)
	if state == 1 and wld_list then
		cur_name = str
	end
end

local function Scale()
	if not wld_ent or not cur_name then return end
	--need wld_ent, selected name, desired scale factor, and wld_list
	local scale
	local input = iup.text{visiblecolumns = 5, mask = iup.MASK_FLOAT}
	local getscale
	local but = iup.button{title = "Done", action = function() scale = tonumber(input.value); getscale:hide() end}
	getscale = iup.dialog{iup.vbox{
		iup.label{title = "Please enter the desired scale factor for this model (relative to 1)"},
		input, but, gap = 12, nmargin = "15x15", alignment = "ACENTER"};
		k_any = function(self, key) if key == iup.K_CR then but:action() end end}
	iup.Popup(getscale)
	iup.Destroy(getscale)
	if scale then
		local s, err = pcall(wld.Scale, wld_ent, cur_name, scale, wld_list)
		if s then
			s, err = pcall(eqg.WriteDirectory, open_path, open_dir)
			if s then
				local msg = iup.messagedlg{title = "Success", value = "Successfully scaled model ".. cur_name .." by ".. scale .."."}
				iup.Popup(msg)
				iup.Destroy(msg)
				return
			end
		end
		error_popup(err)
	end
end

local function AdjustZ()
	if not wld_ent or not cur_name then return end
	--need wld_ent, selected name, desired scale factor, and wld_list
	local adj
	local input = iup.text{visiblecolumns = 5, mask = iup.MASK_FLOAT}
	local get
	local but = iup.button{title = "Done", action = function() adj = tonumber(input.value); get:hide() end}
	get = iup.dialog{iup.vbox{
		iup.label{title = "Please enter the desired Z adjustment:"},
		input, but, gap = 12, nmargin = "15x15", alignment = "ACENTER"};
		k_any = function(self, key) if key == iup.K_CR then but:action() end end}
	iup.Popup(get)
	iup.Destroy(get)
	if adj then
		local s, err = pcall(wld.AdjustZ, wld_ent, cur_name, adj, wld_list)
		if s then
			s, err = pcall(eqg.WriteDirectory, open_path, open_dir)
			if s then
				local msg = iup.messagedlg{title = "Success", value = "Successfully adjusted model ".. cur_name .." Z by ".. adj .."."}
				iup.Popup(msg)
				iup.Destroy(msg)
				return
			end
		end
		error_popup(err)
	end
end

function list:button_cb(button, pressed, x, y)
	if button == iup.BUTTON3 and pressed == 0 then
		local mx, my = iup.GetGlobal("CURSORPOS"):match("(%d+)x(%d+)")
		local active = cur_name and "YES" or "NO"
		local menu = iup.menu{
			iup.item{title = "Scale Model", action = Scale, active = active},
			iup.item{title = "Adjust Model Z", action = AdjustZ, active = active},
		}
		iup.Popup(menu, mx, my)
		iup.Destroy(menu)
	end
end

return iup.vbox{list; alignment = "ACENTER", gap = 5}

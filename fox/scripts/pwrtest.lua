local ret = makescript()

function run()
	local spos, wait, state = 0, 0, "servo"
	exec("set srange 46 188")
--	exec("set slave 0")
--    exec("set speed 65 65")
	while true do
		if wait < gettimems() then
			if state == "servo" then
        		robot.setservo(spos)
        		spos = spos + 45
        		if spos > 180 then
        			spos = 0
	        		state = "move"
	    			wait = gettimems() + 1500
    			else
	    			wait = gettimems() + 1000
        		end
       		elseif state == "move" then
       		    exec("set speed 80 80")
       		    wait = gettimems() + 3000
       		    state = "startservo"
   		    elseif state == "startservo" then
   		        robot.stop()
   		        wait = gettimems() + 500
   		        state = "servo"
        	end
        end
		coroutine.yield()
	end
end

function finish()
	robot.stop()
end

return ret


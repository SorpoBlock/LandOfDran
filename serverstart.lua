t = {}
for x = 0, 7, 1 do
	for y = 0, 7, 1 do
		idx = x * 7 + y
		t[idx] = createDynamic(0,x*3.5,5,y*3.5)
	end
end

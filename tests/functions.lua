function test_function()
	print("This is from test_function()")
end

test_function()


print("Nested call test")

function foo()
	print("This is from foo()")
end
function bar()
	print("This is from bar()")
end

function foobar()
	foo()
	print("foo() above, bar() below; from foobar()")
	bar()
end

foobar()
print("Called foobar()")


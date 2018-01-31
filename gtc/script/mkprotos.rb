#!/usr/bin/ruby

class String
    def scan_for_structenum(target)
	scan(/(struct\s+\w+)/) do
	    target[$1] = true
	end
	scan(/enum\((\w+)\)/) do
	    target["enum #$1"] = true
	end
    end
end

class FuncDef
    attr_accessor :name,:files,:proto,:referenced
    def initialize(name,files,proto)
	@name = name
	@files = files
	@proto = proto
	@referenced = []
    end
    def to_s
	(files.length==1 ? files[0] : "{"+files.join(",")+"}")+":#{name} [[#{proto}]]"
    end
    def inspect
	"{{FuncDef #{to_s}}}"
    end
    def compatible?(oth)
	name==oth.name && proto==oth.proto
    end
    def static?
	proto =~ /^\bstatic\b/
    end
end

funcs = {}
structenum = {}

while x=gets
    if x !~ /^(\w+):(.*?):(.*)$/
	throw x
    else
	func = FuncDef.new($1,[$2],$3)
	if !func.static?
	    func.proto.scan_for_structenum(structenum)
	    name = func.name
	    if funcs[name]
		if funcs[name].compatible?(func)
		    funcs[name].files += func.files
		else
		    $stderr.puts "conflict between #{funcs[name]} and #{func}"
		    throw :conflict
		end
	    else
		funcs[name] = func
	    end
	end
    end
end

banlist = %w(debug isidch isalnum isdigit isspace unary declare nl Compile err_int warn_int error_do AP_app AP_about sUnpack)
banlist.each do |name|
    funcs.delete(name)
end

class Union
    include Enumerable
    def initialize(*sets)
	@sets = sets
    end
    def each
	@sets.each do |set|
	    set.each do |item|
		yield item
	    end
	end
    end
end

Union.new(Dir['**/*.c']).each do |path|
    File.open(path) do |f|
	words = Hash.new 0
	f.each do |line|
	    line.scan(/\w+/) do |word|
		if funcs[word]
		    words[word] += 1
		end
	    end
	end
	words.each do |word,count|
	    if funcs[word].files != [path]
		funcs[word].referenced << path
	    end
	end
    end
end


puts "// Auto-generated file, do not edit!"
puts
puts "#ifndef PROTOS_H_"
puts "#define PROTOS_H_"
puts
structenum.keys.sort.each do |decl|
    puts "#{decl};"
end
puts
funcs.each do |name,func|
    if !func.referenced.empty? || func.files!=["src/out68k_as.c"]
	puts func.proto
    end
end
puts
puts "#endif"

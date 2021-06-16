

module Complete

@alternatives = nil

#def initialize
#    @commands = ["ls", "cd", "cat"]
#end

# Return the longest common prefix from an array of strings;
# ie ['aax', 'aap', 'aaa'] => 'aa'
def longest_common_prefix(strs)
  return '' if strs.empty?
  min, max = strs.minmax
  idx = min.size.times{ |i| break i if min[i] != max[i] }
  min[0...idx]
end

# 'Accept' the current completion and clear the current list
def accept()
    @alternatives = nil
end

def alternatives
    return @alternatives
end

# Split ie Class::method.other.last
def split_reference(ref)
    result = []
    current = ''
    last_t = false
    ref.each_char do |c|
        t = '.:'.include?(c) 
        if t == last_t
            current << c
        else
            result << current unless current.empty?
            current = c
            last_t = t
        end
    end
    result << current unless current.empty?
    result
end


# Complete `line` using current state
# Algorithm:
# * Scan backwards until start of expression is found.
# * Split expression into symbols and separators.
# * Check sym.return_type(:sym1), etc until we fail (no return type)
#   or reach the end.
# * If we didn't fail we have 'obj' + 'incomplete'
# ex
# Module::Class.method
# method(arrgs..).
def complete(full_line)

    p full_line

    # We can't handle strings so lets not even try
    return "" if full_line.include?('"') or full_line.include?("'")
    
    # Scan backwards from line end until we found the start of the
    # last 'symbol reference'; ie anything consisting of symbols
    # separated by dots or double colons
    i = full_line.size-1
    while i >= 0
        c = full_line[i]
        if ('a'..'z') === c or ('A'..'Z') === c or
           ('0'..'9') === c or '.$@_:'.include?(c)
            i -= 1
            next
        end
        if c == ')' and i > 0 and full_line[i-1] == '('
            i -= 2
            next
        end
        break
    end
    p "I:" + i.to_s

    # Prefix is ignored when completing, but added back when returning
    prefix = i < 0 ? "" : full_line[0..i]
    # line is what we want to complete
    line = full_line[i+1..-1]

    p "LINE:" + line
    p "PREF:" + prefix

    if prefix == "ls "
        @alternatives = list_files()
    end

    parts = split_reference(line)
    incomplete = parts.last
    objname = nil
    first = ''
    p parts
    if parts.size > 2
        objname = parts[0]
        first = parts[0..-2].join
    end

    parts.each do |p|
    end


    p "FIRST:" + first
    p "OBJ:" + (objname ? objname : '')
    p "INCOMPLETE:" + incomplete
    obj = objname ? eval(objname) : nil
    p obj

    if @alternatives.nil?
        if obj.nil?
            if incomplete[0] == '$'
                full_list = global_variables().map(&:to_s)
            elsif incomplete[0] >= 'A' && incomplete[0] <= 'Z'
                full_list = []
                ObjectSpace.each_object(Module) { |o|
                    s = o.to_s
                    full_list.append(s) if s[0] != '#' && !s.include?(':')
                }
                #p full_list
            else
                full_list = self.public_methods().map(&:to_s)
            end
        else
            full_list = obj.send("public_methods", true).map(&:to_s)
            if obj.class == Module
                full_list += obj.send("constants").map(&:to_s)
            end
        end

        $ano = 0
        @alternatives = full_list.select { |a| a.start_with?(incomplete) }
        if @alternatives.empty?
            @alternatives = full_list.select { |a| a.include?(incomplete) }
            return "" if @alternatives.empty?
            result = @alternatives[$ano % @alternatives.size]
            $ano += 1
        else
            result = longest_common_prefix(@alternatives)
        end
    else
        return "" if @alternatives.empty?
        result = @alternatives[$ano % @alternatives.size]
        $ano += 1
    end

    @alternatives.sort!

    p @alternatives

    return prefix + (obj ? first : "") + result

end


end

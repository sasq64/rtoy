

module Complete

@alternatives = nil

def initialize
    @commands = ["ls", "cd", "cat"]
end

def longest_common_prefix(strs)
  return '' if strs.empty?
  min, max = strs.minmax
  idx = min.size.times{ |i| break i if min[i] != max[i] }
  min[0...idx]
end


def compl(str, alternatives)
    matching = alternatives.select { |a| a.start_with?(str) }
    longest_common_prefix(matching)
end

def accept()
    p "ACCEPT"
    @alternatives = nil
end


# Complete `line` using current state
def complete(full_line)

    p full_line

    # We can't handle strings so lets not even try
    return "" if full_line.include?('"') or full_line.include?("'")
    
    # Scan backwards from line end until we found the start of the
    # last 'symbol reference'; ie anyting consisting of symbols
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


    # Scan backwards again to see if we have a complete object
    # reference that we should evaluate. If we do, it goes into
    # `objname`.
    # `incomplete` is the string we should complete, and is either
    # a member of `objname` or a "global".
    i = line.length-1
    while i > 0
        break if line[i] == '.' or line[i] == ':'
        i -= 1
    end
    if i != 0 
        incomplete = line[i+1..]
        first = line[0..i]
        while i > 0
            break if line[i] != '.' and line[i] != ':'
            i -= 1
        end
        objname = line[0..i]
    else
        objname = ''
        first = ''
        incomplete = line
    end



    p "FIRST:" + first
    p "OBJ:" + objname
    p "INCOMPLETE:" + incomplete
    obj = i == 0 ? nil : eval(objname)
    p obj

    if @alternatives.nil?
        if obj.nil?
            if incomplete[0] == '$'
                full_list = global_variables().map(&:to_s)
            elsif incomplete[0] >= 'A' && incomplete[0] <= 'Z'
                full_list = []
                ObjectSpace.each_object(Class) { |o|
                    s = o.to_s
                    full_list.append(s) if s[0] != '#'
                }
                ObjectSpace.each_object(Module) { |o|
                    s = o.to_s
                    full_list.append(s) if s[0] != '#'
                }
                p full_list
            else
                full_list = self.public_methods().map(&:to_s)
            end
        else
            full_list = obj.send("public_methods", true).map(&:to_s)
            if obj.class == Module
                full_list += obj.send("constants").map(&:to_s)
            end
            p full_list
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

    return prefix + (obj ? first : "") + result

end


end

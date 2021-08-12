
module MethAttrs

    def init()
        if !@return_types
            @return_types = {}
            @arg_types = {}
            @doc_strings = {}

            @returns = nil
            @doc_string = nil
            @file_name = nil
            @takes_file = nil
            @class_doc = nil
        end
    end

    def method_added(name)
        if @doc_string
            p "#{name}:\n#{@doc_string}\n"
            @doc_strings[name] = @doc_string
            @doc_string = nil
        end
        if @returns
            p "#{name} returns #{@returns}"
            @return_types[name] = @returns
            @returns = nil
        end
        if @takes_file
            @arg_types[name] = :file
            @takes_file = nil
        end
    end

    def doc!(doc_string, meth = nil)
        init()
        if meth
            @doc_strings[meth] = doc_string
        else
            @doc_string = doc_string
        end
    end

    def class_doc!(doc_string)
        @class_doc = doc_string
    end

    def returns!(type, meth = nil)
        init()
        if meth
            @return_types[meth] = type
        else
            @returns = type
        end
    end

    def takes_file!
        @file_name = true
    end

    def takes_file?(method)
        return @arg_types[method] == :file
    end

    def get_return_type(method)
        type = @return_types[method]
        p ">> RET for #{method} is #{type}"
        type
    end

    def get_doc(meth = nil)
        meth.nil? ? @class_doc : @doc_strings[meth]
    end
end


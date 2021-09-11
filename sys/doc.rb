
module Doc
    def doc(cls)

        while cls.class != Class and cls.class != Module
            cls = cls.class
        end

        console.fg = Color::YELLOW
        #puts "class #{cls}"
        has_doc = cls.respond_to?(:get_doc) 
        doc = has_doc ? cls.get_doc() : nil
        if doc
            console.fg = Color::GREY
            puts doc
        end
        methods = cls.instance_methods(false)
        if cls.respond_to?(:superclass) && cls.superclass == Layer
            methods += cls.superclass.instance_methods(false)
        end
        setters = []
        methods.each do |method|
            m = method.to_s
            if m[-1] == '='
                setters << m[0...-1]
            end
        end
        setters.each do |s| 
            methods.delete((s + '=').to_sym)
            if methods.include?(s.to_sym)
                methods.delete(s.to_sym)
            end
        end
        p setters

        console.fg = Color::YELLOW
        puts "METHODS"
        methods.each do |m|
            console.fg = Color::WHITE
            params = cls.instance_method(m).parameters
            txt = nil
            params.each do |param|
                if txt
                    txt += ","
                else
                    txt = ""
                end
                txt += param[1].to_s
            end
            txt = "" if !txt
            print "    " + m.to_s + "(" + txt + ")"
            doc = has_doc ? cls.get_doc(m) : nil
            if doc
                console.fg = Color::LIGHT_GREY
                puts " - " + doc
            end
            puts
        end
        console.fg = Color::YELLOW
        puts "ATTRIBUTES"
        setters.each do |m|
            console.fg = Color::WHITE
            puts "    " + m.to_s
            doc = cls.get_doc(m)
            if doc
                console.fg = Color::LIGHT_GREY
                puts doc
            end
        end
        console.fg = Color::WHITE

    end
end

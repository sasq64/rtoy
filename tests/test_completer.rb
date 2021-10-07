require 'minitest/autorun'

def list_files(s)
    Dir.entries(s)
end

#require_relative 'ruby/os.rb'
require_relative 'ruby/complete.rb'

class MyCoolClass
    def hey_people
    end
    def goodbye
    end
end

module MyCoolModule
    class Sub
        def my_fun_method()
        end
    end

    def self.my_mod_method()
    end

    def self.get_class
        nil
    end

    def self.returns(s)
        puts s
        puts s.class
        return MyCoolClass if s == :get_class
        puts "FAIL"
        return nil
    end



end

class TestCompleter < MiniTest::Test

    include Complete

    def test_filename
        f = complete_file('ru')
        assert_equal '"ruby"', f
        f = complete_file('ruby/r')
        assert_equal '"ruby/re"', f
        f = complete_file('ruby/co')
        assert_equal '"ruby/complete.rb"', f
    end

    def test_split
        s = split_reference 'Hello::method.more.go'
        p s
    end

    def test_operators
        accept()
        #l = complete('MyCoolClass.<')
    end

    def test_simple_completion
        l = complete('puts MyC')
        assert_equal "puts MyCool", l
        alternatives().each { |a|
            assert a.start_with?('MyC')
        }

        l = complete(l)
        assert_equal "puts MyCoolClass", l
        l = complete(l)
        assert_equal "puts MyCoolModule", l
        accept()
        l = complete(l + ".my")
        assert_equal l, "puts MyCoolModule.my_mod_method"
        accept()
        assert_equal "", complete(l + ".other")
        accept()

        assert_equal "puts MyCoolModule.get_class.hey_people",
            complete("puts MyCoolModule.get_class.hey")

    end
end

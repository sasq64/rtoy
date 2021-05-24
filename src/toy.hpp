
#include <memory>
#include <mruby.h>
#include <mruby/data.h>
#include <mruby/value.h>
#include <string>

#include <fstream>

class Toy
{
    template <typename Stream>
    static std::string read_all(Stream& in)
    {
        std::string contents;
        auto here = in.tellg();
        in.seekg(0, std::ios::end);
        contents.resize(in.tellg() - here);
        in.seekg(here, std::ios::beg);
        in.read(&contents[0], contents.size());
        in.close();
        return contents;
    }

    mrb_state* ruby;
public:

    void init();
    void destroy();

    bool render_loop();
    int run(std::string const& script);
};

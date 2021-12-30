# tween() should create a new tween. Needs at least 'seconds' to know
# how long it will live.
# Targets are added to a tween. Each target is an object/method pair,
# along with a function that translates 'delta' into arguments for
# the method
#

class TweenError < StandardError
end

class TweenFunc

    def self.in_back(t)
        s = 1.70158
        (s+1)*t*t*t - s*t*t
    end

    def self.out_back(t)
        s = 1.70158
        t -= 1
        (t*t*((s+1)*t + s) + 1)
    end

    def self.smooth_step(t)
        t*t*(3-2*t)
    end

    def self.linear(t)
        t
    end

    def self.in_sine(t)
        1 - Math::cos(t * (Math::PI/2))
    end

    def self.out_sine(t)
        Math::sin(t * (Math::PI/2))
    end

    def self.in_out_sine(t)
        -0.5 * (Math::cos(Math::PI*t) - 1)
    end

    def self.out_bounce(t)
       n1 = 7.5625;
       d1 = 2.75;
       if t < 1 / d1
           n1 * t * t
       elsif t < 2 / d1
           n1 * (t -= 1.5 / d1) * t + 0.75
       elsif t < 2.5 / d1
           n1 * (t -= 2.25 / d1) * t + 0.9375
       else
           n1 * (t -= 2.625 / d1) * t + 0.984375
       end
    end

    def self.in_out_bounce(t)
        t < 0.5 ? (1 - out_bounce(1 - 2 * t)) / 2
                : (1 + out_bounce(2 * t - 1)) / 2
    end

    def self.in_bounce(t)
        1 - out_bounce(1-t)
    end

    def self.in_cubic(t)
        t*t*t
    end

    def self.out_cubic(t)
        1 - (1-t)**3
    end

    def self.in_out_cubic(t)
        t < 0.5 ? 4*t*t*t : 1 - ((-2*t+2)**3)/2
    end

    def self.in_circ(t)
        1 - Math::sqrt(1 - t*t)
    end

    def self.out_circ(t)
        Math::sqrt(1 - (t-1)*(t-1))
    end

    def self.in_out_circ(t)
        t < 0.5 ? (1 - Math::sqrt(1 - (2*t)**2)) / 2 :
            (Math::sqrt(1 - (-2 * t + 2)**2) + 1) / 2
    end

    def self.sine(t)
        (Math::sin(t * (Math::PI*2) - Math::PI/2) + 1.0)/2.0
    end

    def self.in_elastic(t)
        c4 = (2 * Math::PI) / 3
        return t if t == 0 || t == 1
        -2**(10 * t - 10) * Math::sin((t * 10 - 10.75) * c4)
    end

    def self.out_elastic(t)
        c4 = (2 * Math::PI) / 3
        t = 2**(-10 * t) * Math::sin((t * 10 - 0.75) * c4) + 1
        t
    end

    def self.in_out_elastic(t)
        c5 = (2 * Math::PI) / 4.5
        t === 0 ? 0 : t === 1 ? 1 : t < 0.5 ?
        -(2**(20 * t - 10) * Math::sin((20 * t - 11.125) * c5)) / 2
        : (2**(-20 * t + 10) * Math::sin((20 * t - 11.125) * c5)) / 2 + 1
    end
end

class DeltaTarget
    def initialize(obj, method, from, delta)
        @obj = obj
        @method = method
        @from = from
        @value = from.clone
        @callback = nil
        @type = nil
        @delta = delta
        if @from.nil?
            @delta = nil
        elsif @from.kind_of? Array
            @type = :array
        else
            @type = :scalar
        end
    end

    def set_callback(cb)
        @callback = cb
    end

    def update(delta)
        if @type == :scalar
            @value += @delta
            @obj.send @method, @value
        elsif @type == :array
            (0...@from.size).each { |i|
                @value[i] += @delta[i]
            }
            @obj.send @method, @value
        end
        done = delta >= 1.0
        @callback.call(self) if done and @callback
        done
    end
end

class TweenTarget

    def initialize(obj, method, from, to, seconds, func)
        @obj = obj
        @method = method
        @from = from
        @value = from.clone
        @func = func
        @callback = nil
        @type = nil
        if @from.nil?
            @delta = nil
        elsif @from.kind_of? Array
            @type = :array
            @diff = to
            (0...@diff.size).each { |i| @diff[i] -= @from[i] }
        else
            @type = :scalar
            @diff = to - @from
        end
    end

    def set_callback(cb)
        @callback = cb
    end

    def update(delta)
        d = TweenFunc.send @func, delta
        if @type == :scalar
            @value = @from + @diff * d
            @obj.send @method, @value
        elsif @type == :array
            (0...@from.size).each { |i|
                @value[i] = @from[i] + @diff[i] * d
            }
            @obj.send @method, @value
        else
            @obj.send @method,d
        end

        done = delta >= 1.0
        @callback.call(self) if done and @callback
        done
    end
end

class Tween
    @@tweens = []
    @@seconds = Timer.default.seconds

    LEGALS = [ :obj, :method, :func, :seconds, :from, :to ]
    REQUIRED = [ :obj, :method, :seconds, :from, :to ]

    def initialize(o = nil, seconds: 1.0, &block)
        @block = block
        @func = :linear
        @targets = []
        seconds = o if o.kind_of? Numeric
        @obj = o || obj
        @total_time = seconds
        @start_time = @@seconds

    end

    def done?
        @targets.all { |a| a.delta >= 1.0 }
    end

    def object()
        @obj
    end

    def add_target(r)
        REQUIRED.each { |x|
            raise TweenError.new "Missing attribute :#{x} for tween" unless r[x] 
        }

        raise TweenError.new "Unknown easing :#{r[:func]}" unless 
            Symbol === r[:func] && TweenFunc.respond_to?(r[:func])

        @targets.append TweenTarget.new(
            r[:obj], r[:method], r[:from], r[:to],
            r[:seconds], r[:func])
        self
    end

    def seconds(s, &block)
        @block = block if block
        @total_time = s
        self
    end

    def fn(f)
        @func = f
        self
    end

    def delta(**kwargs)
        r = {
            obj: @obj,
            seconds: @total_time,
        }

        delta = 0
        kwargs.each do |key,val|
            r[:from] = r[:obj].send(key)
            val = val.to_a if val.respond_to?(:to_a)
            r[:from] = r[:from].to_a if r[:from].respond_to?(:to_a)
            r[:method] = [key, '='].join.to_sym
            delta = val
        end
        @targets.append DeltaTarget.new(
            r[:obj], r[:method], r[:from], delta)
        self
    end
    def to(**kwargs)
        r = {
            obj: @obj,
            func: @func,
            seconds: @total_time,
        }

        kwargs.each do |key,val|
            r[:from] = r[:obj].send(key)
            val = val.to_a if val.respond_to?(:to_a)
            r[:to] = val
            r[:from] = r[:from].to_a if r[:from].respond_to?(:to_a)
            r[:to] = r[:from] - r[:from] + val
            r[:method] = [key, '='].join.to_sym
        end
        add_target(r)
        self
    end

    def from(**kwargs)
        r = {
            obj: @obj,
            func: @func,
            seconds: @total_time,
        }

        kwargs.each do |key,val|
            r[:to] = r[:obj].send(key)
            r[:to] = r[:to].to_a if r[:to].respond_to?(:to_a)
            val = val.to_a if val.respond_to?(:to_a)
            r[:from] = r[:to] - r[:to] + val
            r[:method] = [key, '='].join.to_sym
        end
        add_target(r)
        self
    end

    def when_done(&block)
        @targets.last.set_callback(block)
        self
    end

    def every(d, &block)
        #@targets.last.set_every_cb(d, block)
        self
    end

    def update(t)
        delta = @total_time ? (@@seconds - @start_time) / @total_time : 0
        delta = 1.0 if delta > 1.0
        res = true
        if delta >= 0
            if @block
                @block.call(delta)
                res = (delta >= 1.0)
            end
            @targets.each { |tg| res &= tg.update(delta) }
            @on_done.call if @on_done && res
        end
        res
    end

    def finish
        @start_time -= @total_time
        @targets.each { |tg| tg.update(1.0) } 
        @on_done.call if @on_done
        @@tweens.delete self
    end

    def stop
        @obj = nil
    end

    # Called from handler
    def self.update_all(delta)
        Display.default.bench_start(0)
        @@seconds = Timer.default.seconds
        @@tweens.delete_if do |tw|
            tw.update(delta)
        end
        Display.default.bench_end(0)
    end

    def self.clear()
        @@tweens = []
    end

    def self.start(*args, &block)
        tween = Tween.new(*args, &block)
        @@tweens.append(tween)
        tween
    end

    def start()
        @start_time = @@seconds
        @@tweens.append(self)
        self
    end

    def self.make(*args, &block)
        Tween.new(*args, &block)
    end

    def self.show_all
        w,h = 150,150
        gap = 20
        x,y = gap,gap
        OS.display.clear
        TweenFunc.public_methods(false).each do | fn |
            OS.canvas.fg = Color::BLACK
            OS.canvas.rect(x,y,w,h)
            OS.canvas.fg = Color::GREEN
            (0..w).each do |fx|
                fy = TweenFunc.send(fn, fx.to_f/w)
                fy = ((1 - fy)*0.5 + 0.25) * h
                OS.circle(fx + x,fy + y, 2)
            end
            Fiber.yield
            OS.canvas.fg = Color::WHITE
            OS.canvas.text(x,y, fn.to_s, 20)
            Fiber.yield
            x += w+gap
            if(x + w+gap > OS.display.width)
                x = gap
                y += h+gap
            end
        end
        loop { OS.vsync }
    end

end

def tween(*args, &block)
    Tween.start(*args, &block)
end


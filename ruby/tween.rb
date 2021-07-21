# tween() should create a new tween. Needs at least 'seconds' to know
# how long it will live.
# Targets are added to a tween. Each target is an object/method pair,
# along with a function that translates 'delta' into arguments for
# the method
#

class TweenError < StandardError
end

def by_elem(a, b, method)
    [a, b].transpose.map {|x| x.reduce(method)}
end

class TweenFunc
    def self.ease_out_back(t)
        s = 1.70158
        t -= 1
        (t*t*((s+1)*t + s) + 1);
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
        -0.5 * (Math::cos(Math::PI*t) - 1);
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

    def self.sine(t)
        (Math::sin(t * (Math::PI*2) - Math::PI/2) + 1.0)/2.0;
    end

    def self.in_elastic(t)
        c4 = (2 * Math::PI) / 3;
        t == 0 ? 0 : t == 1 ? 1
        : -2**(10 * t - 10) * Math::sin((t * 10 - 10.75) * c4);
    end

    def self.out_elastic(t)
        c4 = (2 * Math::PI) / 3;
        t == 0 ? 0 : t == 1 ? 1
        : -2**(-10 * t) * Math::sin((t * 10 - 10.75) * c4) + 1;
    end

    def self.in_out_elastic(t)
        c5 = (2 * Math::PI) / 4.5
        t === 0 ? 0 : t === 1 ? 1 : t < 0.5 ?
        -(2**(20 * t - 10) * Math::sin((20 * t - 11.125) * c5)) / 2
        : (2**(-20 * t + 10) * Math::sin((20 * t - 11.125) * c5)) / 2 + 1;
    end
end

class TweenTarget

    def initialize(obj, method, from, to, seconds, steps, func)
        @obj = obj
        @method = method
        @from = from
        @steps = steps - 1
        @to = to
        @value = from
        @func = func
        @callback = nil
    end

    def set_callback(cb)
        @callback = cb
   end

    def update(delta)
        if @steps > 0
            delta = (delta * @steps).to_i / @steps.to_f
        end
        delta = 1.0 if delta > 1.0
        if delta >= 0
            d = TweenFunc.send @func, delta
            if @from.nil?
                @obj.send @method,d
            elsif @from.kind_of? Array
                @value = by_elem(@from, by_elem(@to, @from, :-).
                                 map{|x| x * d}, :+)
                @obj.send @method, @value
            else
                @value = @from + (@to - @from) * d
                @obj.send @method, @value
            end
        end
        done = delta >= 1.0
        @callback.call(self) if done and @callback
        done
    end
end

class Tween
    @@tweens = []

    LEGALS = [ :obj, :method, :func, :seconds, :steps, :from, :to ]
    REQUIRED = [ :obj, :method, :seconds, :from, :to ]

    def initialize(o = nil, seconds: 1.0, &block)
        @block = block
        @func = :linear
        @targets = []
        seconds = o if o.kind_of? Numeric
        @obj = o || obj
        @total_time = seconds
        @start_time = Timer.default.seconds

    end

    def add_target(r)
        REQUIRED.each { |x|
            raise TweenError.new "Missing attribute :#{x} for tween" unless r[x] 
        }

        raise TweenError.new "Unknown easing :#{r[:func]}" unless 
            Symbol === r[:func] && TweenFunc.respond_to?(r[:func])

        @targets.append TweenTarget.new(
            r[:obj], r[:method], r[:from], r[:to],
            r[:seconds], r[:steps], r[:func])
        self
    end

    def seconds(s)
        @total_time = s
        self
    end

    def fn(f)
        @func = f
        self
    end

    # attr: val
    def to(**kwargs)
        r = {
            obj: @obj,
            func: @func,
            seconds: @total_time,
            steps: 0
        }

        kwargs.each do |key,val|
            r[:from] = r[:obj].send(key)
            r[:to] = val
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
            steps: 0
        }

        kwargs.each do |key,val|
            r[:to] = r[:obj].send(key)
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

    def update(t)
        delta = @total_time ? (Timer.default.seconds - @start_time) / @total_time : 0
        res = true
        if @block
            @block.call(delta)
            res = (delta >= 1.0)
        end
        @targets.each { |tg| res &= tg.update(delta) } 
        @on_done.call if @on_done && res
        res
    end

    def finish
        @start_time -= @total_time
    end

    def stop
        @obj = nil
    end

    def self.update_all(delta)
        @@tweens.delete_if do |tw|
            tw.update(delta)
        end
    end

    def self.clear()
        @@tweens = []
    end

    def self.make(*args, &block)

        tween = Tween.new(*args, &block)
        @@tweens.append(tween)
        tween
    end

end

def tween(*args, &block)
    Tween.make(*args, &block)
end


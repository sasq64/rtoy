# tween() should create a new tween. Needs at least 'seconds' to know
# how long it will live.
# Targets are added to a tween. Each target is an object/method pair,
# along with a function that translates 'delta' into arguments for
# the method
#

def by_elem(a, b, method)
    [a, b].transpose.map {|x| x.reduce(method)}
end

class TweenTarget

    def initialize(obj, method, from, to, seconds, steps)
        @obj = obj
        @method = method
        @from = from
        @steps = steps - 1
        @to = to
        @value = from
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
            if @from.nil?
                @obj.send @method
            elsif @from.kind_of? Array
                @value = by_elem(@from, by_elem(@to, @from, :-).
                                 map{|x| x * delta}, :+)
                @obj.send @method,*@value
            else
                @value = @from + (@to - @from) * delta
                p @value
                @obj.send @method,@value
            end
        end
        done = delta >= 1.0
        @callback.call(self) if done and @callback
        done
    end
end

class Tween
    @@tweens = []

    def initialize(o = nil, m = nil, obj:nil, method:nil, on_done: nil,
                   seconds:nil, &block)
        @block = block
        @targets = []
        seconds = o if o.kind_of? Numeric
        @obj = o || obj
        @method = m || method
        @on_done = on_done
        @total_time = seconds
        @start_time = Timer.default.seconds

    end

    def target(o = @obj, m = @method, obj:nil, method:nil, from:nil,
               to_pos: nil, from_pos: nil, to_rot: nil, from_rot: nil,
               to:nil, seconds:1.0, steps:0)
        obj ||= o
        if to_rot
            from = obj.rotation
            to = to_rot
            method = :rotation=
        end
        if from_rot
            to = obj.rotation
            from = from_rot
            method = :rotation=
            obj.rotation = from
        end
        if to_pos
            from = obj.pos
            to = to_pos
            method = :pos=
        end
        if from_pos
            to = obj.pos
            from = from_pos
            method = :pos=
            obj.pos = from
        end
        method ||= m
        @targets.append TweenTarget.new(obj, method, from, to, seconds, steps)
        self
    end

    def when_done(&block)
        @targets.last.set_callback(block)
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


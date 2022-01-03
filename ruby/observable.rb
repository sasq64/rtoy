# frozen_string_literal: true

# Mixin for any class that needs to be observable
module Observable
  # Connect an observer to this observable
  def listen(&block)
    @blocks ||= []
    @blocks << block
  end

  def signal(obj, what, *args)
    @blocks ||= []
    @blocks.each { |b| b.call(obj, what, *args) }
  end
end

module Attributes
  def set(name, val)
    @attrs ||= {}
    @attrs[name] = val
  end

  def get(name)
    @attrs ||= {}
    @attrs[name]
  end
end

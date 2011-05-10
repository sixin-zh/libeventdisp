#! /usr/bin/ruby

# A simple script to automate the benchmarking tests

require "set"

module LibeventdispBenchmark
  TIME_CMD = "/usr/bin/time -f \"%E, %U, %S, %W, %w, %c\""
  CONV_CMD = "./conv"
  CHAR_COUNT_CMD = "./count"

  # Get a number that is divisible to the given numbers
  #
  # Params:
  #  num {Array<Integer>}
  def self.get_divisible(num)
    to_check = num.to_set
    num = 1

    until to_check.empty? do
      next_num = to_check.max

      unless num % next_num == 0 then
        num *= next_num
      end

      to_check.delete next_num
    end

    return num
  end

  # Returns {Array<Integer>} the distributed load ratio for each thread
  def self.get_thread_load(thread_count)
    number = get_divisible(1.upto(thread_count))

    1.upto(thread_count).map do |x|
      number / x
    end
  end

  def self.conv_test(thread_count)
    thread_load = get_thread_load(thread_count)

    test_pattern = [
      [1, 100],
      [1, 1000],
      [10, 1000],
      [100, 1000],
      [1000, 100],
      [1000, 1000],
      [100, 10000],
    ]

    test_pattern.each do |arg|
      puts "conv: #{thread_load[0] * arg[0]} x #{arg[1]}"

      thread_load.each_with_index do |load, idx|
#        puts "#{TIME_CMD} #{CONV_CMD} #{idx + 1} #{load * arg[0]} #{arg[1]}"
        `#{TIME_CMD} #{CONV_CMD} #{idx + 1} #{load * arg[0]} #{arg[1]}`
      end
    end
  end

  def self.char_test(thread_count)
    thread_load = get_thread_load(thread_count)

    test_pattern = [
      100, 1000, 10000, 10000, 1000000, 10000000
    ]

    test_pattern.each do |arg|
      puts "char count: #{thread_load[0]} #{arg}"

      thread_load.each_with_index do |load, idx|
#        puts "#{TIME_CMD} #{CHAR_COUNT_CMD} #{idx + 1} #{load} #{arg}"
        `#{TIME_CMD} #{CHAR_COUNT_CMD} #{idx + 1} #{load} #{arg}`
      end
    end
  end
end

if __FILE__ == $0 then
  if ARGV.size != 1 then
    puts "Usage: #{$0} <number of threads>"
  else
    THREAD_COUNT = ARGV[0].to_i

    LibeventdispBenchmark.conv_test THREAD_COUNT
    LibeventdispBenchmark.char_test THREAD_COUNT
  end
end


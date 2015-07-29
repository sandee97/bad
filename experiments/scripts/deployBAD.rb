#!/usr/bin/env ruby

#
# This script deploys B.A.D to an EC2 machine.
#

require './lib/deploy'

require 'net/ssh'
require 'net/scp'
require 'optparse'

TAR_FILE = 'bad.tar.gz'

options = {:defaults => false}

optparse = OptionParser.new do |opts|
  opts.banner = "Usage: #{$0} [options] [hosts]
    \nDeploy a `bad.tar.gz` to a set of machines\n\n"

  opts.on("-d", "--defaults", "Use default settings") do |key|
    options[:defaults] = true
  end

  options[:tar_file] = TAR_FILE
  opts.on("-t", "--dist-file STRING", "Distribution file to deploy") do |df|
    options[:tar_file] = df
  end
end

if ARGV.length == 0
  optparse.parse %w[--help]
end

optparse.parse!

ARGV.each do |host|
  deployer = Deploy.new(hostname: host)
  if options[:defaults]
    deployer.set_opts(distfile: options[:tar_file], user: 'ubuntu', skey: {})
  end
  deployer.deploy!
end

puts "\nB.A.D deployed!"

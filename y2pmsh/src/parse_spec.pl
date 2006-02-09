#!/usr/bin/perl -w

use strict;

my $fromsystem;

my @macros;
my $dist;

use File::Basename;
push @INC, dirname($0);
require Needed;
import Needed;

use Getopt::Long;
Getopt::Long::Configure("no_ignore_case");

my ($opt_target, $opt_dist, @opt_define, @opt_with, @opt_without);
GetOptions (
    "target=s"   => \$opt_target,
    "dist=s"   => \$opt_dist,
    "define=s"   => \@opt_define,
    "with=s"   => \@opt_with,
    "without=s"   => \@opt_without,
    ) or exit(1);

if($opt_target) {
  Needed::extra_define("_target_cpu $opt_target\n");
}

for my $def (@opt_define) {
  Needed::extra_define("$def\n");
}

for my $def (@opt_with) {
  Needed::extra_define("_with_$def --with-$def\n");
}

for my $def (@opt_without) {
  Needed::extra_define("_without_$def --without-$def\n");
}

if ($opt_dist) {
  my $confdir = '/etc/y2pmbuild';
  $dist = $opt_dist;

  my $dir = $confdir.'/macros/'.$opt_dist;
  if(opendir(F, $dir)) {
    my @files = grep { $_ ne '.' && $_ ne '..' } readdir(F);
    closedir(F);
    push @macros, map { $dir.'/'.$_ } @files;
  }
} else {
  my $target = $opt_target;
  if(!$target) {
    $target = `rpm --eval \%_target`;
    chomp $target;
  }
  $dist = $target;
  if(!($target =~ /-/)) {
    $target .= '-'.`rpm --eval \%_target_os`;
    chomp $target;
  }
  push @macros, "/usr/lib/rpm/$target/macros";
  push @macros, "/usr/lib/rpm/suse_macros";
}

die unless @macros;

Needed::set_macro_files(@macros);
Needed::init_read_spec($dist);

for my $file (@ARGV) {
  my ($name, $vers, $subpacks, @needed) = Needed::read_spec_complex($file);

  print $name, "\n";
  print $vers, "\n";
  print join(' ', @$subpacks), "\n";
  print join(' ', @needed), "\n";
}

# vim: sw=2

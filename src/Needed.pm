# spec file parser by mls

package Needed;

sub unify {
  my %h = map {$_ => 1} @_;
  return grep(delete($h{$_}), @_);
}

my %varmap = ('BUILD_BASENAME' => 'build_basename',
              'BUILDMACHINETYPE' => 'buildmachinetype',
              'ABUILD_USED_LIBC' => 'abuild_used_libc');

sub init {
  my $basename = shift;
  my $machtype = shift;
  my $usedlibc = shift;

  undef @buildpacks;
  undef @basepacks;
  undef @comppacks_all;
  undef %comppacks;
  undef %norecomp;
  undef @preinstalls;
  %skips = ();
  $build_basename = $basename;
  $buildmachinetype = $machtype;
  $abuild_used_libc = $usedlibc;
}

my $simple_macros_dist;
my $read_spec_dist;
my $read_spec_inited;
my $extra_defines;

sub read_spec {
  my $specfile = shift;
  my $xspec = shift;
  local *SPEC;

  if (!$read_spec_inited) {
    die("read_spec_complex: init me first!\n") unless $build_basename;
    init_read_spec($build_basename);
  }
  if (use_read_spec_complex($read_spec_dist)) {
    return read_spec_complex($specfile, $xspec);
  }
  if (ref($specfile) eq 'GLOB') {
    *SPEC = $specfile;
  } elsif (!open(SPEC, "< $specfile")) {
    warn("$specfile: $!\n");
    return (undef, "open: $!", undef);
  }
  my @needed = ();
  my @subpacks = ();
  my $packname;
  my $packvers;
  my $badsubpack = 0;
  while (<SPEC>) {
    if (/^#\s*neededforbuild\s*(\S.*)$/) {
      push @needed, split(/\s+/, $1);
    } elsif (!defined($packname) && /^\s*Name:\s*(\S+)/) {
      push @subpacks, $1;
      $packname = $1;
    } elsif (!defined($packvers) && /^\s*Version:\s*(\S+)/) {
      $packvers = $1;
      $packvers = 'MACRO' if $packvers =~ /^%/;
    } elsif (/^%package\s+(-n\s+)?(\S+)/) {
      if (!defined($packname)) {
        $badsubpack = 1;
	next;
      }
      push @subpacks, $1 ? $2 : "$packname-$2";
    }
  }
  close SPEC;
  @subpacks = unify(@subpacks);
  unshift @subpacks, undef if $badsubpack;
  s/\.rpm$// for @needed;
  return ($packname, $packvers, \@subpacks, @needed);
}

###########################################################################
my @macros;

sub set_macro_files {
  @macros = @_;
}

sub extra_define {
  $extra_defines = '' unless $extra_defines;
  $extra_defines .= "%define ".shift;
}

sub extract_simple_macros {
  my $dist = shift;

  my $mac = '';
  for my $macro (@macros) {
    if (!open(F, '<', $macro)) {
      warn("$macro: $!\n");
      next;
    }
    while (<F>) {
      chomp;
      next if /^\s*$/;
      next if /^\s*#/;
      if (/\\$/) {
	while (<F>) {
	  last unless /\\$/;
	}
	next;
      }
      next unless /^\s*%([0-9a-zA-Z_]+)(\([^\)]*\))?\s*(.*?)$/;
      my ($macname, $macargs, $macbody) = ($1, $2, $3);
      next if defined $macargs;
      next if $macbody =~ /^%\(/;
      $mac .= "%define $macname $macbody\n";
    }
    close F;
  }
  return $mac;
}

sub expr {
  my $expr = shift;
  my $lev = shift;

  $lev ||= 0;
  my ($v, $v2);
  $expr =~ s/^\s+//;
  my $t = substr($expr, 0, 1);
  if ($t eq '(') {
    ($v, $expr) = expr(substr($expr, 1), 0);
    return undef unless defined $v;
    return undef unless $expr =~ s/^\)//;
  } elsif ($t eq '!') {
    ($v, $expr) = expr(substr($expr, 1), 0);
    return undef unless defined $v;
    $v = !$v;
  } elsif ($t eq '-') {
    ($v, $expr) = expr(substr($expr, 1), 0);
    return undef unless defined $v;
    $v = -$v;
  } elsif ($expr =~ /^([0-9]+)(.*?)$/) {
    $v = $1;
    $expr = $2;
  } elsif ($expr =~ /^([a-zA-Z_0-9]+)(.*)$/) {
    $v = "\"$1\"";
    $expr = $2;
  } elsif ($expr =~ /^(\".*?\")(.*)$/) {
    $v = $1;
    $expr = $2;
  } else {
    return;
  }
  while (1) {
    $expr =~ s/^\s+//;
    if ($expr =~ /^&&/) {
      return ($v, $expr) if $lev > 1;
      ($v2, $expr) = expr(substr($expr, 2), 1);
      return undef unless defined $v2;
      $v &&= $v2;
    } elsif ($expr =~ /^\|\|/) {
      return ($v, $expr) if $lev > 1;
      ($v2, $expr) = expr(substr($expr, 2), 1);
      return undef unless defined $v2;
      $v &&= $v2;
    } elsif ($expr =~ /^>=/) {
      return ($v, $expr) if $lev > 2;
      ($v2, $expr) = expr(substr($expr, 2), 2);
      return undef unless defined $v2;
      $v = (($v =~ /^\"/) ? $v ge $v2 : $v >= $v2) ? 1 : 0;
    } elsif ($expr =~ /^>/) {
      return ($v, $expr) if $lev > 2;
      ($v2, $expr) = expr(substr($expr, 1), 2);
      return undef unless defined $v2;
      $v = (($v =~ /^\"/) ? $v gt $v2 : $v > $v2) ? 1 : 0;
    } elsif ($expr =~ /^<=/) {
      return ($v, $expr) if $lev > 2;
      ($v2, $expr) = expr(substr($expr, 2), 2);
      return undef unless defined $v2;
      $v = (($v =~ /^\"/) ? $v le $v2 : $v <= $v2) ? 1 : 0;
    } elsif ($expr =~ /^</) {
      return ($v, $expr) if $lev > 2;
      ($v2, $expr) = expr(substr($expr, 1), 2);
      return undef unless defined $v2;
      $v = (($v =~ /^\"/) ? $v lt $v2 : $v < $v2) ? 1 : 0;
    } elsif ($expr =~ /^==/) {
      return ($v, $expr) if $lev > 2;
      ($v2, $expr) = expr(substr($expr, 2), 2);
      return undef unless defined $v2;
      $v = (($v =~ /^\"/) ? $v eq $v2 : $v == $v2) ? 1 : 0;
    } elsif ($expr =~ /^!=/) {
      return ($v, $expr) if $lev > 2;
      ($v2, $expr) = expr(substr($expr, 2), 2);
      return undef unless defined $v2;
      $v = (($v =~ /^\"/) ? $v ne $v2 : $v != $v2) ? 1 : 0;
    } elsif ($expr =~ /^\+/) {
      return ($v, $expr) if $lev > 3;
      ($v2, $expr) = expr(substr($expr, 1), 3);
      return undef unless defined $v2;
      $v += $v2;
    } elsif ($expr =~ /^-/) {
      return ($v, $expr) if $lev > 3;
      ($v2, $expr) = expr(substr($expr, 1), 3);
      return undef unless defined $v2;
      $v -= $v2;
    } elsif ($expr =~ /^\*/) {
      ($v2, $expr) = expr(substr($expr, 1), 4);
      return undef unless defined $v2;
      $v *= $v2;
    } elsif ($expr =~ /^\//) {
      ($v2, $expr) = expr(substr($expr, 1), 4);
      return undef unless defined $v2 && 0 + $v2;
      $v /= $v2;
    } else {
      return ($v, $expr);
    }
  }
}

my $std_macros = q{%define ix86 i386 i486 i586 i686 athlon
%define arm armv4l armv4b armv5l armv5b armv5tel armv5teb
%define arml armv4l armv5l armv5tel
%define armb armv4b armv5b armv5teb
};

sub use_read_spec_complex {
  return 1;
}

sub init_read_spec {
  my $dist = shift;
  if (!use_read_spec_complex($dist)) {
    $simple_macros_dist = '';
    $read_spec_dist = $dist;
    $read_spec_inited = 1;
    return;
  }
  $simple_macros_dist = $std_macros;
  $simple_macros_dist .= $extra_defines if $extra_defines;
  $simple_macros_dist .= "%define _target_os linux\n";
  $simple_macros_dist .= extract_simple_macros($dist) if $dist;
  $read_spec_dist = $dist;
  $read_spec_inited = 1;
}

sub read_spec_complex {
  my $specfile = shift;
  my $xspec = shift;

  if (!$read_spec_inited) {
    die("read_spec_complex: init me first!\n") unless $build_basename;
    init_read_spec($build_basename);
  }
  my $macros = $simple_macros_dist;
  my $packname;
  my $packvers;
  my @subpacks;
  my @packdeps;
  my $hasnfb;
  my $nfbline;
  my %macros;
  my @xreq;

  local *SPEC;
  if (ref($specfile) eq 'GLOB') {
    *SPEC = $specfile;
  } elsif (!open(SPEC, '<', $specfile)) {
    warn("$specfile: $!\n");
    return (undef, "open: $!", undef);
  }
  my @macros = split("\n", $macros || '');
  print STDERR map { $_." <<< \n" } @macros;
  my $skip = 0;
  my $main_preamble = 1;
  my $inspec = 0;
  while (1) {
    my $line;
    if (@macros) {
      $line = pop @macros;
    } else {
      $inspec = 1;
      $line = <SPEC>;
      last unless defined $line;
      chomp $line;
      push @$xspec, $line if $xspec;
    }
    if ($line =~ /^#\s*neededforbuild\s*(\S.*)$/) {
      next if $hasnfb;
      $hasnfb = $1;
      $nfbline = $#$xspec if $inspec && $xspec;
      next;
    }
    if ($line =~ /^\s*#/) {
      next unless $line =~ /^#!BuildIgnore/;
    }
    my $expandedline = '';
    if (!$skip) {
      my $tries = 0;
      # {
      while ($line =~ /^(.*?)%(\{([^\}]+)\}|[0-9a-zA-Z_]+|%|\()(.*?)$/) {
	if ($tries++ > 1000) {
	  $line = 'MACRO';
	  last;
	}
	$expandedline .= $1;
	my $macname = defined($3) ? $3 : $2;
	$line = $4;
	my $mactest;
	if ($macname =~ /^\!\?/ || $macname =~ /^\?\!/) {
	  $mactest = -1;
	} elsif ($macname =~ /^\?/) {
	  $mactest = 1;
	}
	$macname =~ s/^[\!\?]+//;
	my $macalt;
	($macname, $macalt) = split(':', $macname, 2);
	if ($macname eq '%') {
	  $expandedline .= '%';
	  next;
	} elsif ($macname eq '(') {
	  $line = 'MACRO';
	  last;
	} elsif ($macname eq 'define') {
	  if ($line =~ /^\s*([0-9a-zA-Z_]+)(\([^\)]*\))?\s*(.*?)$/) {
	    my $macname = $1;
	    my $macargs = $2;
	    my $macbody = $3;
	    $macbody = undef if $macargs;
	    $macros{$macname} = $macbody;
	  }
	  $line = '';
	  last;
	} elsif (exists($macros{$macname})) {
	  if (!defined($macros{$macname})) {
	    $line = 'MACRO';
	    last;
	  }
	  $macalt = $macros{$macname} unless defined $macalt;
	  $macalt = '' if $mactest && $mactest == -1;
	  $line = "$macalt$line";
	} elsif ($mactest) {
	  $macalt = '' if !defined($macalt) || $mactest == 1;
	  $line = "$macalt$line";
	} else {
	  $expandedline .= "%$2";
	}
      }
    }
    $line = $expandedline . $line;
    if ($line =~ /^\s*%else\b/) {
      $skip = !$skip if $skip < 2;
      next;
    }
    if ($line =~ /^\s*%endif\b/) {
      $skip-- if $skip;
      next;
    }
    $skip++ if $skip && $line =~ /^\s*%if/;

    next if $skip;

    if ($line =~ /^\s*%ifarch(.*)$/) {
      my $arch = $macros{'_target_cpu'} || 'unknown';
      my @archs = grep {$_ eq $arch} split(/\s+/, $1);
      $skip = 1 if !@archs;
      next;
    }
    if ($line =~ /^\s*%ifnarch(.*)$/) {
      my $arch = $macros{'_target_cpu'} || 'unknown';
      my @archs = grep {$_ eq $arch} split(/\s+/, $1);
      $skip = 1 if @archs;
      next;
    }
    if ($line =~ /^\s*%ifos(.*)$/) {
      my $os = $macros{'_target_os'} || 'unknown';
      my @oss = grep {$_ eq $os} split(/\s+/, $1);
      $skip = 1 if !@oss;
      next;
    }
    if ($line =~ /^\s*%ifnos(.*)$/) {
      my $os = $macros{'_target_os'} || 'unknown';
      my @oss = grep {$_ eq $os} split(/\s+/, $1);
      $skip = 1 if @oss;
      next;
    }
    if ($line =~ /^\s*%if(.*)$/) {
      my ($v, $r) = expr($1);
      $v = 0 if $v && $v eq '\"\"';
      $skip = 1 unless $v;
      next;
    }
    if ($main_preamble && ($line =~ /^Name:\s*(\S+)/i)) {
      $packname = $1;
      $macros{'name'} = $packname;
    }
    if ($main_preamble && ($line =~ /^Version:\s*(\S+)/i)) {
      $packvers = $1;
      $macros{'version'} = $packvers;
    }
    if ($main_preamble && ($line =~ /^(BuildRequires|\#\!BuildIgnore):\s*(\S.*)$/i)) {
      my $what = $1;
      my $deps = $2;
      my @deps = $deps =~ /([^\s\[\(,]+)(\s+[<=>]+\s+[^\s\[,]+)?(\s+\[[^\]]+\])?[\s,]*/g;
      if (defined($hasnfb)) {
        next unless $xspec;
        if ((grep {$_ eq 'glibc' || $_ eq 'rpm' || $_ eq 'gcc' || $_ eq 'bash'} @deps) > 2) {
          # ignore old generetad BuildRequire lines.
	  pop @$xspec;
        }
        next;
      }
      my $replace = 0;
      my @ndeps = ();
      while (@deps) {
	my ($pack, $vers, $qual) = splice(@deps, 0, 3);
	if (defined($qual)) {
          $replace = 1;
          my $arch = $macros{'_target_cpu'} || '';
          my $proj = $macros{'_target_project'} || '';
	  $qual =~ s/^\s*\[//;
	  $qual =~ s/\]$//;
	  my $isneg = 0;
	  my $bad;
	  for my $q (split('[\s,]', $qual)) {
	    $isneg = 1 if $q =~ s/^\!//;
	    $bad = 1 if !defined($bad) && !$isneg;
	    if ($isneg) {
	      if ($q eq $arch || $q eq $proj) {
		$bad = 1;
		last;
	      }
	    } elsif ($q eq $arch || $q eq $proj) {
	      $bad = 0;
	    }
	  }
	  next if $bad;
	}
	push @ndeps, $pack;
      }
      $replace = 1 if grep {/^-/} @ndeps;
      if ($what eq '#!BuildIgnore') {
	push @packdeps, map {"-$_"} @ndeps;
	next;
      }
      push @packdeps, @ndeps;
      next unless $xspec && $inspec;
      if ($replace) {
        if (0) {
	  $xspec->[-1] = "#!__*$xspec->[-1]";
        } else {
	  pop @$xspec;
        }
	my @cndeps = grep {!/^-/} @ndeps;
        if (0) {
	  push @$xspec, "%?__*BuildRequires: ".join(' ', @cndeps) if @cndeps;
        } else {
	  push @$xspec, "BuildRequires: ".join(' ', @cndeps) if @cndeps;
        }
	push @xreq, $#$xspec if @cndeps;
      } elsif ($xspec) {
	push @xreq, $#$xspec;
      }
      next;
    }

    if ($line =~ /^\s*%package\s+(-n\s+)?(\S+)/) {
      if ($1) {
	push @subpacks, $2;
      } else {
	push @subpacks, "$packname-$2" if defined $packname;
      }
    }

    if ($line =~ /^\s*%(package|prep|build|install|check|clean|preun|postun|pretrans|posttrans|pre|post|files|changelog|description|triggerpostun|triggerun|triggerin|trigger|verifyscript)/) {
      $main_preamble = 0;
    }
  }
  if (defined($hasnfb)) {
    if (!@packdeps) {
      @packdeps = split(' ', $hasnfb);
      unshift @xreq, $nfbline if defined $nfbline;
    }
  }
  unshift @subpacks, $packname;
  if ($xspec) {
    my %dontdelete = map {$_ => 1} get_dontdelete();
    my @xnegs = map {"-$_"} grep {!$dontdelete{$_}} @subpacks;
    modify_spec($xspec, \@xreq, @packdeps, @xnegs);
  }
  return ($packname, $packvers, \@subpacks, @packdeps);
}

sub get_dontdelete {
	print STDERR "get_dontdelete\n";
}

sub modify_spec {
	print STDERR "modify_spec\n";
}

1;

#!/usr/bin/env perl
use warnings;
use strict;
use 5.010;
use Carp;

use YAML::Any qw/ Dump LoadFile /;

=for later

This tool generates the bidirectional thunks between g++'s stack-based calling
and AmigaOS's LVO based calling.

AmigaOS libraries are modelled as C++ classes, with the AmigaOS library
functions being class methods. This nicely means that the library base is to
hand for placing into %a6.

The generated thunks fall into these categories:

A list of method declarations for #including into the class definition, e.g.:

 void ExecBase::Insert(MinList *list, MinNode *node, MinNode *listnode);

A list of method definitions, optionally inlinable, which performs the thunking
from C++ to assembly, e.g.:

 void ExecBase::Insert(MinList *list, MinNode *node, MinNode *listnode) {
    register const ExecBase *_a6 asm ("%a6") = this;
    register MinList *_a0 asm ("%a0") = list;
    register MinNode *_a1 asm ("%a1") = node;
    register MinNode *_a2 asm ("%a2") = listnode;
    asm  ("jsr -234(%0)"
          :
          : "r"(_a6), "r"(_a0), "r"(_a1), "r"(_a2)
          : "%d0", "%d1", "%cc"
        );
}

A list of plain functions that go the other way:

 void Insert() {
   register MinList *list asm("%a0");
   register MinNode *node asm("%a1");
   register MinNode *listnode asm("%a2");
   listnode ? node->insert_after(listnode) : list->unshift(node);
 };

g++4.4 can cope with the asm-to-C++ all being in one file, whereas g++4.5
aggressively optimises and eliminates function bodies of functions returning
void, thus requiring a slightly different hack of one function per file:

 register MinNode *listnode asm("a2");
 register MinList *list asm("a0");
 register MinNode *node asm("a1");
 void Insert() {
   listnode ? node->insert_after(listnode) : list->unshift(node);
 }

It would also be useful to generate the function table for
exec.library/MakeFunctions, since we already have all that data to hand.

=cut

my $library = LoadFile('exec.yml');

use Data::Dump qw/ pp /;
my $global = delete $library->{global}
    // croak "No global section";
my $class = delete $global->{class}
    // croak "No class";
my $impl_preamble = delete $global->{impl_preamble}
    // croak "No impl_preamble";
my $impl_postamble = delete $global->{impl_postamble}
    // croak "No impl_postamble";
my $impl_namespace = delete $global->{impl_namespace}
    // croak "No impl_postamble";
my $module = delete $global->{module}
    // croak "No module";
# my $header = delete $global->{header}
#     // croak "No global header";
# my $path = delete $global->{path}
#     // croak "No global path";

sub mapin {
    my($description) = @_;
    my($before, $var, $reg, $after) =
        ($description =~ /^(.*?){(\w+?)=(\w+?)}(.*?)$/)
            or croak "Can't parse $_";
    return {
        reg => $reg,
        var => $var,
        type => "$before $after",
        typevar => "$before$var$after",
        format => "$before%s$after",
    };
}

my $outfmt = "src/gen/exec";
system ('mkdir', '-p', $outfmt);
open CDEC, '>', "$outfmt.cdec.inc" or croak;
open CDEF, '>', "$outfmt.cdef.inc" or croak;
open MAKE, '>', "$outfmt/module.mk" or croak;
open VECS, '>', "$outfmt.vecs.inc" or croak;
open VDEF, '>', "$outfmt.vdef.inc" or croak;

print MAKE <<"EOT";
# -*- makefile -*-
# This file is autogenerated. Do not edit.

${module}_SRC += \\
EOT

my %offsets;

foreach my $function (sort keys %$library) {
    print "Generating stubs for $function\n";
    my %fn = %{ $library->{$function} };
    my $in = delete $fn{in}
        // croak "'in' not set";
    my @in = ref $in ? @$in : ($in);
    @in = () if $in[0] eq 'void';
    my $out = delete $fn{out}
        // croak "'out' not set";
    @in = map mapin($_), @in;
    my @out;
    if (defined $out && $out ne 'void') {
        $out = mapin($out);
        push @out, $out;
    } else {
        $out = { type => 'void' };
    }

    my $offset = delete $fn{offset}
        // croak "missing offset";
    croak "invalid offset $offset"
        if $offset >= 0 || $offset % 6;
    #pp +{ $function => \%fn };

    #my $code = delete $fn{code} // "/* FIXME */";
    #my $code = delete $fn{code} // next;
    #croak "No code for $function" unless defined $code;
    my $code = delete $fn{code};

    #undef $code; # don't generate any thunks for now

    my $static = delete $fn{static};

    # c++ to asm definition

    printf CDEC "$out->{type} $function(";
    if (@in) {
        print CDEC join ', ',
            map +(join ' ', $_->{typevar}), @in;
    } else {
        print CDEC 'void';
    }
    # we need noinline to avoid exciting gcc bugs
    //print CDEC ") __attribute__((noinline));\n";
    print CDEC ") __attribute__;\n";

    # c++ to asm declaration
    printf CDEF "$out->{type} ${class}::$function(";
    if (@in) {
        print CDEF join ', ',
            map +(join ' ', $_->{typevar}), @in;
    } else {
        print CDEF 'void';
    }
    print CDEF ") {\n";

    push @in, {
        reg => 'a6',
        var => 'this',
        type => "$class *",
        format => "$class *%s",
    };

    foreach my $in (@in) {
        print CDEF qq~  register @{[ sprintf $in->{format}, "in_$in->{reg}" ]} asm("%$in->{reg}") = $in->{var};\n~;
    }
    foreach my $out (@out) {
        print CDEF qq~  register @{[ sprintf $out->{format}, "out_$out->{reg}" ]} asm("%$out->{reg}");\n~;
    }
    my %clobber = map +($_ => $_), qw/ d0 d1 a0 a1 cc /;
    delete @clobber{ map $_->{reg}, @in, @out };
    print CDEF qq~  asm ("jsr $offset(%%a6)"~;
    # asm out list
    print CDEF qq~ : ~, join(", ", map qq~"=r"(out_$_->{reg})~, @out);
    # asm in list
    print CDEF qq~ : ~, join(", ", map qq~"r"(in_$_->{reg})~, @in);
    # asm clobber list
    print CDEF qq~ : ~, join(", ", map qq~"%$_"~, sort keys %clobber), ");\n";
    print CDEF "  return out_$out->{reg};\n"
        if @out;
    print CDEF "}\n\n";

    pop @in;                    # remove the unshifted a6 from above

    # c++ to asm declaration

    my $type = join ', ', map $_->{type}, @in;
    $type ||= 'void';

    unless($static) {
        push @in, {
            reg => 'a6',
            var => lc $class,
            type => "$class *",
            format => "$class *%s",
            #typevar => "$class *\L$class",
        };
    }

    my %in = map +($_->{reg}, $_), @in;

    if (defined $code && $code !~ /\A\s*external\s*\Z/) {
        my $outsrc = "$outfmt/$function.cpp";
        #my $outsrc = sprintf "%s/%04x.cpp", $outfmt, $offset & 0xffff;
        open IMPL, '>', $outsrc or croak "Can't create $outsrc: $!";
        print MAKE "\t$outsrc \\\n";
        print IMPL $impl_preamble;
        foreach my $reg (qw/ a6 /) {
            my $in = delete $in{$reg}
                or next;
            print IMPL qq~register @{[ sprintf $in->{format}, "$in->{var}" ]} asm("%$in->{reg}");\n~;
        }
        printf IMPL "$out->{type} ${impl_namespace}$function(...) {\n";
        foreach my $in (values %in) {
            print IMPL qq~register @{[ sprintf $in->{format}, "$in->{var}" ]} asm("%$in->{reg}");\n~;
        }
        print IMPL qq~  $code;\n~;
        print IMPL "}\n\n";
        print IMPL $impl_postamble;
        close IMPL;
    }
    if(defined $code) {
        printf VDEF qq~static $out->{type} $function(...) asm ("exec\$$function");\n~;
    }
    croak "Offset $offset already used"
        if exists $offsets{$offset};
    $offsets{$offset} = $code ? "reinterpret_cast<int32_t>(&$impl_namespace$function)" : "0 /* $function is not yet implemented */";
}

my $offset = 0;
while (scalar keys %offsets) {
    $offset -= 6;
    croak "Too many offsets!\n" . Dump %offsets
        if $offset < -32768;
    if (my $function = delete $offsets{$offset}) {
        print VECS "  $function,\n";
    } else {
        print VECS "  0 /* no function at this offset */,\n";
    }
}

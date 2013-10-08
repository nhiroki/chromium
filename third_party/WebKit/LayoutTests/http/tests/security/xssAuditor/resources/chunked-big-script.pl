#!/usr/bin/perl -wT

my $pre_chunked = ($ENV{"SERVER_SOFTWARE"} && index($ENV{"SERVER_SOFTWARE"}, "LightTPD") != -1);

sub print_maybe_chunked
{
    my $string = shift;
    if ($pre_chunked) {
        print $string;
        return;
    }
    printf "%lx\r\n", length($string);
    print "$string\r\n";
}

print "Content-type: text/html\r\n";
if (!$pre_chunked) {
    print "Transfer-encoding: chunked\r\n";
}
print "\r\n";
print_maybe_chunked "<!DOCTYPE html><html><script>var a; var b; a=1,a=1,a=1,a=1,a=1,a=1,a=1,a";
for ($count = 0; $count < 256; $count++) {
    print_maybe_chunked "=1,a=1,a=1,a=1,a=1,a=1,a=1,a=1,a=1,a=1,a=1,a=1,a=1,a=1,a=1,a=1,a=1,a=1,a";
}
print_maybe_chunked "=1;</script>The End</html>\r\n";
print_maybe_chunked "";

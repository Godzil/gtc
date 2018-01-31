#!/usr/bin/perl

$out=<>;
$out=~s/\r//g;
chomp $out;
-e $out and die "file already exists, cowardly exiting";
open OUT,">",$out or die "failed to write file";
$param=<>;
$param=~s/\r//g;
chomp $param;
die "extended merges aren't supported" if $param!='';
while (<>) {
	chomp;
	s/\r//g;
	open FILE,"<",$_ or die "failed to open file";
	print OUT while <FILE>;
	close FILE;
}
close OUT;

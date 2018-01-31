#!/usr/bin/perl

my $multicomment=0;

while (<>) {
	chomp;
	if (!$multicomment) {
		s|//.*$| |g;
		s|/\*.*?\*/| |g;
		if (m|/\*|) {
			s|/\*.*$| |g;
			$multicomment=1;
		}
	} elsif (m|\*/|) {
		s|^.*?\*/| |g;
		$multicomment=0;
	} else {
		$_='';
	}
	s|^\s+||g;
	s|\s+$||g;
	s|\s+| |g;
	s|^(\#define \w+) |\1\t|g;
	s|(\W) (.)|\1\2|g;
	s|(.) (\W)|\1\2|g;
	s:(?<!\w)0x([A-Fa-f0-9]{1,5})(?!(\#|\d)):hex($1):ge;
	while (s:(?<!\w)(\d+)\*(\d+)(?!(\#|\d)):$1*$2:ge) {}
	s|\t| |g;
	$_.="\n" if $_;
	print;
}

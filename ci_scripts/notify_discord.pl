#!/usr/bin/perl

use strict;
use utf8;
use JSON;
use LWP::UserAgent;
use LWP;

if (not defined $ENV{APPVEYOR_PROJECT_SLUG}) {
	# test
	$ENV{APPVEYOR_BUILD_WORKER_IMAGE} = "Image Name";
	$ENV{APPVEYOR_PROJECT_SLUG} = "teraterm";
	$ENV{APPVEYOR_REPO_NAME} = "APPVEYOR_REPO_NAME";
	$ENV{APPVEYOR_REPO_BRANCH} = "APPVEYOR_REPO_BRANCH";
	$ENV{APPVEYOR_REPO_COMMIT} = "APPVEYOR_REPO_COMMIT";
	$ENV{APPVEYOR_REPO_COMMIT} = "31";
	#$ENV{WEBHOOK_URL} = "https://discord.com/api/....";
}

#print "ENV{APPVEYOR}=$ENV{APPVEYOR}\n";
#print "ENV{WEBHOOK_URL}=$ENV{WEBHOOK_URL}\n";

if (not defined $ENV{APPVEYOR}) {
	print "Do not run on Appveyor\n";
	exit 0;
}
if (not defined $ENV{WEBHOOK_URL}) {
	print "\$WEBHOOK_URL is empty\n";
	exit 0;
}

my $hook = $ENV{WEBHOOK_URL};
my $result = $ARGV[0];
my $color;
if ($result =~ /success/) {
	$color = "255";			# 0x0000ff green
} else {
	$color = "16711680";	# 0xff0000 red
}
my $image = $ENV{APPVEYOR_BUILD_WORKER_IMAGE};

my $revision = "$ENV{APPVEYOR_REPO_COMMIT}";
my $rev_url = "https://osdn.net/projects/ttssh2/scm/svn/commits/$revision";
my $artifacts_url = "https://ci.appveyor.com/project/$ENV{APPVEYOR_ACCOUNT_NAME}/$ENV{APPVEYOR_PROJECT_SLUG}/build/job/$ENV{APPVEYOR_JOB_ID}/artifacts";

my $description = <<"EOS";
$ENV{COMPILER_FRIENDLY}/$ENV{COMPILER}
Commit \[r$revision\]($rev_url) by $ENV{APPVEYOR_REPO_COMMIT_AUTHOR} on $ENV{APPVEYOR_REPO_COMMIT_TIMESTAMP}
\[Download\]($artifacts_url)
EOS

my %json= (
	"content" => "build $result r$revision",
	"embeds" => [{
#		"title" => "",
		"color" => $color,
		"description" => "$description",
		}]
	);

#print encode_json(\%json);

if(1) {
	# LWP
	my $browser = LWP::UserAgent->new;

	my $client = HTTP::Request->new(POST => $hook);
	$client->content_type('application/json');
	$client->header("User-Agent" => "DiscordBot");

	$client->content(encode_json(\%json));

	my $response = $browser->request($client);
	if ($response->is_success) {
		print $response->content;
	}
	else {
		print $response->status_line, "\n";
	}

} else {
	# curl
	my $curl = 'curl -H "Accept: application/json" -H "Content-type: application/json" -X POST -d';
	my $cmd=$curl . " '" . encode_json(\%json) . "' '" . $hook . "'";
	system($cmd);
}

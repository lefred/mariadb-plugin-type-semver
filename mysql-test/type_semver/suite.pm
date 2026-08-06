package My::Suite::TypeSemver;
@ISA = qw(My::Suite);
return "Not run for embedded server" if $::opt_embedded_server;
return "No TYPE_SEMVER plugin" unless $ENV{TYPE_SEMVER_SO};
bless { };

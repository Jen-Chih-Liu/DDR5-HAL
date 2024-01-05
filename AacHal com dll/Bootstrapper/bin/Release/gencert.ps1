$cert = New-SelfSignedCertificate -DnsName test.name.com -CertStoreLocation cert:\LocalMachine\My -type CodeSigning
$pwd = ConvertTo-SecureString -String "helloworld" -Force -AsPlainText
Export-PfxCertificate -cert $cert -FilePath mycert.pfx -Password $pwd

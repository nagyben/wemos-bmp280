terraform {
    source = "${get_terragrunt_dir()}/../terraform//app"
}

include "root" {
  path = find_in_parent_folders()
  expose = true
}

inputs = {
    env_suffix = "-test"
    ci_sa = include.root.locals.terraform_service_account
}
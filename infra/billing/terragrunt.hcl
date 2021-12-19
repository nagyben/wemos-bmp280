terraform {
    source = "${get_terragrunt_dir()}/../terraform//billing"
}

include "root" {
  path = find_in_parent_folders()
}
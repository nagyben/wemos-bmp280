terraform {
  backend "gcs" {
    bucket = "nagyben-tfstate"
    prefix = "terraform/state"
  }
}

provider "google" {
  project = "nagyben"
  region  = "europe-west2"
  zone    = "europe-west2-a"
}
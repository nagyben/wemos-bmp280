locals {
  timestamp = formatdate("YYMMDDhhmmss", timestamp())
	root_dir = abspath("../cloud_functions")
}

data "archive_file" "data_ingestion_source" {
  type        = "zip"
  source_dir  = local.root_dir
  output_path = "/tmp/function.zip"
  excludes = [
    "${local.root_dir}/tests",
    "${local.root_dir}/e2e"
  ]
}

resource "google_storage_bucket_object" "zip" {
  # Append file MD5 to force object to be recreated
  name   = "function.zip"
  bucket = google_storage_bucket.functions_storage_bucket.name
  source = data.archive_file.data_ingestion_source.output_path

  keepers {
    md5_hash = "${data.archive_file.data_ingestion_source.output_md5}"
  }
}

# Receiver function
resource "google_cloudfunctions_function" "receiver_function" {
  name    = "receiver"
  runtime = "python39"

  available_memory_mb   = 128
  source_archive_bucket = google_storage_bucket.functions_storage_bucket.name
  source_archive_object = google_storage_bucket_object.zip.name
  trigger_http          = true
  entry_point           = "receiver_function"
}

resource "google_cloudfunctions_function_iam_member" "receiver_function_invoker" {
  project        = google_cloudfunctions_function.receiver_function.project
  region         = google_cloudfunctions_function.receiver_function.region
  cloud_function = google_cloudfunctions_function.receiver_function.name

  role   = "roles/cloudfunctions.invoker"
  member = "allUsers"
}